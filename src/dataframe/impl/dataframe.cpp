#include "dataframe.h"

#include <base/conv/auto_json.h>
#include <cmath>
#include <numeric>
#include <optional>
#include <utility>

#include "../old/datatable.h"

#include "aggregators.h"

namespace Vizzu::dataframe
{
using Refl::unsafe_get;

std::shared_ptr<dataframe_interface> dataframe::copy(
    bool inherit_sorting) const &
{
	auto &&uptr = std::make_unique<dataframe_interface>();
	void *&&data = uptr->data;
	auto &&my_deleter = [data](dataframe *df)
	{
		df->~dataframe();
		::operator delete(data, df);
	};
	using ptr_t = std::unique_ptr<dataframe,
	    std::remove_reference_t<decltype(my_deleter)>>;

	const auto *&&cp = get_if<source_type::copying>(&source);
	return {uptr.release(),
	    [newly = ptr_t{new (data) dataframe(
	                       cp ? cp->other
	                          : unsafe_get<source_type::owning>(
	                              source),
	                       cp && cp->pre_remove ? &*cp->pre_remove
	                                            : nullptr,
	                       cp && inherit_sorting && cp->sorted_indices
	                           ? &*cp->sorted_indices
	                           : nullptr),
	         std::move(my_deleter)}](dataframe_interface *df) mutable
	    {
		    newly.reset();
		    std::default_delete<dataframe_interface>{}(df);
	    }};
}

std::shared_ptr<dataframe_interface> dataframe::create_new()
{
	auto &&uptr = std::make_unique<dataframe_interface>();
	auto my_deleter = [p = uptr.get()](dataframe *df)
	{
		df->~dataframe();
		::operator delete(p, df);
	};
	auto &&uptr2 = std::unique_ptr<dataframe, decltype(my_deleter)>{
	    new (uptr->data) dataframe(),
	    std::move(my_deleter)};
	return {uptr.release(),
	    [rm = std::move(uptr2)](dataframe_interface *df) mutable
	    {
		    rm.reset();
		    std::default_delete<dataframe_interface>{}(df);
	    }};
}

dataframe::dataframe(std::shared_ptr<const data_source> other,
    std::vector<bool> const *filtered,
    std::vector<std::size_t> const *sorted) :
    source(std::in_place_index<1>, std::move(other))
{
	auto &cp = unsafe_get<source_type::copying>(source);
	if (filtered) cp.pre_remove.emplace(*filtered);
	if (sorted) cp.sorted_indices.emplace(*sorted);
	if (!cp.other->finalized.empty())
		state_data.emplace<state_type::finalized>();
}

void valid_unexistent_aggregator(
    const dataframe::any_aggregator_type &agg)
{
	if (!agg
	    || (*agg != aggregator_type::count
	        && *agg != aggregator_type::exists))
		error();
}

void valid_dimension_aggregator(
    const dataframe::any_aggregator_type &agg)
{
	using enum aggregator_type;
	if (agg) switch (*agg) {
		case sum:
		case min:
		case max:
		case mean: error();
		default: break;
		}
}

void valid_measure_aggregator(
    const dataframe::any_aggregator_type &agg)
{
	if (!agg || *agg == aggregator_type::distinct) error();
}

std::string dataframe::set_aggregate(const std::string_view &series,
    const any_aggregator_type &aggregator) &
{
	change_state_to(state_type::aggregating,
	    state_modification_reason::needs_series_type);

	switch (get_data_source().get_series(series)) {
		using enum series_type;
	default: valid_unexistent_aggregator(aggregator); break;
	case dimension: valid_dimension_aggregator(aggregator); break;
	case measure: valid_measure_aggregator(aggregator); break;
	}

	change_state_to(state_type::aggregating,
	    state_modification_reason::needs_own_state);

	auto &&ser = get_data_source().get_series(series);
	data_source::aggregating_type &aggs =
	    unsafe_get<state_type::aggregating>(state_data);

	if (ser == series_type::dimension && !aggregator) {
		if (!aggs.dims
		         .emplace(unsafe_get<series_type::dimension>(ser))
		         .second)
			error();
		return {};
	}

	auto &&[name, uniq] =
	    aggs.add_aggregated(std::move(ser), aggregator.value());
	if (!uniq) error();
	return name;
}

void dataframe::set_sort(const std::string_view &series,
    any_sort_type sort,
    na_position na_pos) &
{
	change_state_to(state_type::sorting,
	    state_modification_reason::needs_series_type);

	auto ser = get_data_source().get_series(series);

	switch (ser) {
		using enum series_type;
	default: error();
	case dimension: {
		std::optional<std::vector<std::size_t>> indices;
		if (const auto &dim = unsafe_get<dimension>(ser).second;
		    (sort == sort_type::by_categories
		        || std::ranges::is_sorted(
		            indices.emplace(dim.get_indices(sort))))
		    && (na_pos == dim.na_pos || !dim.contains_nav))
			break;

		error();
	}
	case measure:
		switch (sort) {
		default:
		case sort_type::less:
		case sort_type::greater: break;
		case sort_type::natural_less:
		case sort_type::natural_greater:
		case sort_type::by_categories: error();
		}
		break;
	}

	change_state_to(state_type::sorting,
	    state_modification_reason::needs_own_state);

	std::get_if<data_source::sorting_type>(&state_data)
	    ->emplace_back(std::move(ser), sort, na_pos);
}

void dataframe::set_custom_sort(
    const std::function<std::weak_ordering(record_type, record_type)>
        &custom_sort) &
{
	if (!custom_sort) return;

	change_state_to(state_type::sorting,
	    state_modification_reason::needs_own_state);

	error();
}

void dataframe::add_dimension(
    std::span<const char *const> dimension_categories,
    std::span<const std::uint32_t> dimension_values,
    std::string_view name,
    adding_type adding_strategy,
    std::span<const std::pair<const char *, const char *>> info) &
{
	change_state_to(state_type::modifying,
	    state_modification_reason::needs_series_type);

	switch (get_data_source().get_series(name)) {
	default: {
		if (adding_strategy == adding_type::override_full
		    || adding_strategy
		           == adding_type::override_all_with_rotation)
			error();

		change_state_to(state_type::modifying,
		    state_modification_reason::needs_own_state);

		unsafe_get<state_type::modifying>(state_data)
		    .emplace_back(name);

		unsafe_get<source_type::owning>(source)->add_new_dimension(
		    dimension_categories,
		    dimension_values,
		    name,
		    info);
		break;
	}
	case series_type::dimension: {
		if (adding_strategy == adding_type::create_or_throw) error();

		change_state_to(state_type::modifying,
		    state_modification_reason::needs_own_state);

		auto &dims = unsafe_get<series_type::dimension>(
		    unsafe_get<source_type::owning>(source)->get_series(name))
		                 .second;

		switch (adding_strategy) {
		case adding_type::create_or_throw:
		case adding_type::create_or_add: {
			dims.add_more_data(dimension_categories,
			    dimension_values);
			break;
		}
		case adding_type::create_or_override:
		case adding_type::override_full: {
			dims.categories.assign(dimension_categories.begin(),
			    dimension_categories.end());
			dims.values.assign(dimension_values.begin(),
			    dimension_values.end());
			dims.info = {info.begin(), info.end()};
			break;
		}
		case adding_type::override_all_with_rotation: {
			dims.categories.assign(dimension_categories.begin(),
			    dimension_categories.end());
			for (auto i = std::size_t{}; i < dims.values.size(); ++i)
				dims.values[i] =
				    dimension_values[i % dimension_values.size()];
			dims.info = {info.begin(), info.end()};
			break;
		}
		}
		break;
	}
	case series_type::measure: error();
	}
}

void dataframe::add_measure(std::span<const double> measure_values,
    std::string_view name,
    adding_type adding_strategy,
    std::span<const std::pair<const char *, const char *>> info) &
{
	change_state_to(state_type::modifying,
	    state_modification_reason::needs_series_type);

	switch (get_data_source().get_series(name)) {
	default: {
		if (adding_strategy == adding_type::override_full
		    || adding_strategy
		           == adding_type::override_all_with_rotation)
			error();

		change_state_to(state_type::modifying,
		    state_modification_reason::needs_own_state);

		unsafe_get<state_type::modifying>(state_data)
		    .emplace_back(name);

		unsafe_get<source_type::owning>(source)->add_new_measure(
		    measure_values,
		    name,
		    info);
		break;
	}
	case series_type::measure: {
		if (adding_strategy == adding_type::create_or_throw) error();

		change_state_to(state_type::modifying,
		    state_modification_reason::needs_own_state);

		auto &meas = unsafe_get<series_type::measure>(
		    unsafe_get<source_type::owning>(source)->get_series(name))
		                 .second;

		switch (adding_strategy) {
		case adding_type::create_or_throw:
		case adding_type::create_or_add: {
			meas.values.insert(meas.values.end(),
			    measure_values.begin(),
			    measure_values.end());
			break;
		}
		case adding_type::create_or_override:
		case adding_type::override_full: {
			meas.values.assign(measure_values.begin(),
			    measure_values.end());
			meas.info = {info.begin(), info.end()};
			break;
		}
		case adding_type::override_all_with_rotation: {
			for (auto i = std::size_t{}; i < meas.values.size(); ++i)
				meas.values[i] =
				    measure_values[i % measure_values.size()];
			meas.info = {info.begin(), info.end()};
			break;
		}
		}
		break;
	}
	case series_type::dimension: error();
	}
}

void dataframe::add_series_by_other(std::string_view,
    std::string_view,
    const std::function<cell_value(record_type, cell_value)> &,
    std::span<const std::pair<const char *, const char *>>) &
{
	if (as_if()) error();
}

void dataframe::remove_series(
    std::span<const std::string_view> names) &
{
	if (names.empty()) return;

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_series_type);

	std::vector<std::size_t> remove_dimensions;
	std::vector<std::size_t> remove_measures;

	for (auto &&s = get_data_source(); const auto &name : names) {
		switch (auto ser = s.get_series(name)) {
			using enum series_type;
		default: error();
		case dimension: {
			auto ix = &unsafe_get<dimension>(ser).second
			        - s.dimensions.data();
			remove_dimensions.insert(
			    std::lower_bound(remove_dimensions.begin(),
			        remove_dimensions.end(),
			        ix),
			    ix);
			break;
		}
		case measure: {
			auto ix =
			    &unsafe_get<measure>(ser).second - s.measures.data();
			remove_measures.insert(
			    std::lower_bound(remove_measures.begin(),
			        remove_measures.end(),
			        ix),
			    ix);
			break;
		}
		}
	}

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_own_state);

	unsafe_get<source_type::owning>(source)->remove_series(
	    remove_dimensions,
	    remove_measures);
}

void dataframe::add_record(std::span<const char *const> values) &
{
	change_state_to(state_type::modifying,
	    state_modification_reason::needs_series_type);

	const auto *vec = get_if<state_type::modifying>(&state_data);
	if (!vec || vec->empty() || vec->size() != values.size()) error();

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_own_state);

	auto &s = *unsafe_get<source_type::owning>(source);
	s.normalize_sizes();

	std::vector<double> measures(s.measure_names.size());
	std::vector<std::string_view> dimensions(
	    s.dimension_names.size());
	for (const auto *it = values.data(); const auto &col : *vec) {
		switch (auto &&ser = s.get_series(col)) {
			using enum series_type;
		default:
		case dimension:
			dimensions[&unsafe_get<dimension>(ser).second
			           - s.dimensions.data()] =
			    *it ? *it : std::string_view{nullptr, 0};
			break;
		case measure:
			char *eof{};
			measures[&unsafe_get<measure>(ser).second
			         - s.measures.data()] = std::strtod(*it, &eof);
			if (eof == *it) error();
			break;
		}
		++it;
	}
	for (std::size_t measureIx{}; const double &measure : measures) {
		auto &mea = s.measures[measureIx++];
		mea.values.emplace_back(measure);
		if (std::isnan(measure)) mea.contains_nan = true;
	}
	for (std::size_t dimensionIx{};
	     const std::string_view &dimension : dimensions) {
		s.dimensions[dimensionIx++].add_element(dimension);
	}
}

void dataframe::remove_records(
    std::span<const std::size_t> record_ids) &
{

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_sorted_records);

	if (!std::ranges::is_sorted(record_ids)) error();

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_own_state);

	unsafe_get<source_type::owning>(source)->remove_records(
	    record_ids);
}

void dataframe::remove_records(
    const std::function<bool(const record_type &)> &filter) &
{
	if (!filter) return;

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_record_count);

	auto count = get_record_count();
	if (count == 0) return;

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_sorted_records);

	std::vector<std::size_t> remove_ix;

	for (std::size_t i{}, max = get_record_count(); i < max; ++i)
		if (!is_filtered(i) && !filter({as_if(), i}))
			remove_ix.push_back(i);

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_own_state);

	if (auto *p = get_if<source_type::copying>(&source)) {
		if (!p->pre_remove) p->pre_remove.emplace(count);

		for (const std::size_t i : remove_ix)
			(*p->pre_remove)[i] = true;

		return;
	}

	unsafe_get<source_type::owning>(source)->remove_records(
	    remove_ix);
}

void dataframe::remove_unused_categories(std::string_view column) &
{
	change_state_to(state_type::modifying,
	    state_modification_reason::needs_series_type);

	std::vector<bool> usage;
	switch (auto &&ser = get_data_source().get_series(column)) {
		using enum series_type;
	default:
	case measure: error();
	case dimension:
		usage =
		    unsafe_get<dimension>(ser).second.get_categories_usage();
		if (std::all_of(usage.begin(), usage.end(), std::identity{}))
			return;
		break;
	}

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_own_state);

	unsafe_get<series_type::dimension>(
	    unsafe_get<source_type::owning>(source)->get_series(column))
	    .second.remove_unused_categories(std::move(usage));
}

void dataframe::change_data(std::size_t record_id,
    std::string_view column,
    cell_value value) &
{
	const double *d = std::get_if<double>(&value);

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_sorted_records);

	const auto &s = get_data_source();

	if (s.get_record_count() <= record_id) error();

	const series_type st = s.get_series(column);
	switch (st) {
		using enum series_type;
	default: error();
	case measure:
		if (!d) error();
		break;
	case dimension:
		if (d) error();
		break;
	}

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_own_state);

	switch (auto &&ser =
	            unsafe_get<source_type::owning>(source)->get_series(
	                column);
	        st) {
		using enum series_type;
	default:
	case measure:
		unsafe_get<measure>(ser).second.values[record_id] = *d;
		break;
	case dimension:
		unsafe_get<dimension>(ser).second.set(record_id,
		    *std::get_if<std::string_view>(&value));
		break;
	}
}

bool dataframe::has_na(const std::string_view &column) const &
{
	switch (auto &&ser = get_data_source().get_series(column)) {
		using enum series_type;
	default: error();
	case measure: {
		const auto &meas = unsafe_get<measure>(ser).second;
		return meas.contains_nan
		    || get_record_count() > meas.values.size();
	}
	case dimension:
		const auto &dim = unsafe_get<dimension>(ser).second;
		return dim.contains_nav
		    || get_record_count() > dim.values.size();
	}
}

void dataframe::fill_na(std::string_view column, cell_value value) &
{
	const double *d = std::get_if<double>(&value);

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_series_type);

	switch (get_data_source().get_series(column)) {
		using enum series_type;
	default: error();
	case measure:
		if (!d) error();
		if (std::isnan(*d)) error();
		break;
	case dimension:
		if (d) error();
		break;
	}

	change_state_to(state_type::modifying,
	    state_modification_reason::needs_own_state);

	auto count = get_record_count();

	switch (auto &&ser =
	            unsafe_get<source_type::owning>(source)->get_series(
	                column)) {
		using enum series_type;
	default: break;
	case measure: {
		auto &meas = unsafe_get<measure>(ser).second;
		if (std::exchange(meas.contains_nan, false))
			for (auto &v : meas.values)
				if (std::isnan(v)) v = *d;
		if (meas.values.size() < count) meas.values.resize(count, *d);
		break;
	}
	case dimension:
		unsafe_get<dimension>(ser).second.set_nav(
		    *std::get_if<std::string_view>(&value),
		    count);
		break;
	}
}

void dataframe::finalize() &
{
	change_state_to(state_type::finalized,
	    state_modification_reason::needs_own_state);
}

std::string dataframe::as_string() const &
{
	const auto *vec = get_if<state_type::modifying>(&state_data);
	if (!vec || vec->empty()) error();

	std::string res{'['};
	bool first = true;
	for (const auto s = get_data_source(); const auto &name : *vec) {
		if (!std::exchange(first, false)) res += ',';
		Conv::JSONObj obj{res};
		switch (auto &&ser = s.get_series(name)) {
			using enum series_type;
		default: error();
		case dimension: {
			const auto &[name, dim] = unsafe_get<dimension>(ser);
			obj("name", name)("type", "dimension")("unit",
			    "")("length", dim.values.size())("categories",
			    dim.categories);
			break;
		}
		case measure: {
			const auto &[name, mea] = unsafe_get<measure>(ser);
			auto &&[min, max] = mea.get_min_max();
			obj("name", name)("type", "measure")("unit",
			    mea.info.at("unit"))("length", mea.values.size())
			    .nested("range")("min", min)("max", max);
			break;
		}
		}
	}
	res += ']';
	return res;
}

std::span<const std::string> dataframe::get_dimensions() const &
{
	return get_data_source().dimension_names;
}

std::span<const std::string> dataframe::get_measures() const &
{
	return get_data_source().measure_names;
}

std::span<const std::string> dataframe::get_categories(
    std::string_view dimension) const &
{
	switch (auto &&ser = get_data_source().get_series(dimension)) {
		using enum series_type;
	default:
	case measure: error();
	case dimension:
		return unsafe_get<dimension>(ser).second.categories;
	}
}

cell_value dataframe::get_data(record_identifier record_id,
    const std::string_view &column) const &
{
	const auto &s = get_data_source();
	std::size_t ix{};

	if (const std::size_t *ixp = std::get_if<std::size_t>(&record_id);
	    !ixp)
		ix = s.change_record_identifier_type(
		    *std::get_if<0>(&record_id));
	else if (const auto *cp = get_if<source_type::copying>(&source);
	         cp && cp->sorted_indices)
		ix = (*cp->sorted_indices)[*ixp];
	else
		ix = *ixp;

	return s.get_data(ix, column);
}

bool dataframe::is_filtered(std::size_t record_id) const &
{
	const auto *cp = get_if<source_type::copying>(&source);
	return cp && cp->pre_remove && (*cp->pre_remove)[record_id];
}

std::string dataframe::get_record_id_by_dims(std::size_t my_record,
    std::span<const std::string> dimensions) const &
{
	return get_data_source().get_id(my_record, dimensions);
}

dataframe::series_meta_t dataframe::get_series_meta(
    const std::string &id) const
{
	using enum state_type;
	const auto *state = get_if<modifying>(&state_data);
	if (!state || state->empty()) error();

	auto it = std::find(state->begin(), state->end(), id);

	if (it == state->end()) error();

	auto ix = static_cast<std::size_t>(it - state->begin());

	switch (auto &&ser = get_data_source().get_series(id)) {
		using enum series_type;
	default: return {{}, ix, ser};
	case measure:
		return {unsafe_get<measure>(ser).first, ix, measure};
	case dimension:
		return {unsafe_get<dimension>(ser).first, ix, dimension};
	}
}

void dataframe::migrate_data()
{
	auto *s = get_if<source_type::copying>(&source);
	switch (state_data) {
		using enum state_type;
	case aggregating: {
		source = std::make_shared<data_source>(
		    std::move(*std::get_if<data_source::aggregating_type>(
		        &state_data)),
		    s && s->pre_remove ? &*s->pre_remove : nullptr,
		    get_record_count());

		break;
	}
	case sorting: {
		if (s) {
			if (auto &&indices = s->other->get_sorted_indices(
			        std::exchange(unsafe_get<sorting>(state_data),
			            {}));
			    !std::ranges::is_sorted(indices))
				s->sorted_indices = std::move(indices);
			else
				s->sorted_indices.reset();
		}
		[[fallthrough]];
	}
	default:
	case modifying:
	case finalized: {
		if (!s) return;
		source = std::make_shared<data_source>(s->other,
		    std::exchange(s->pre_remove, {}),
		    std::exchange(s->sorted_indices, {}));
		break;
	}
	}
}

void dataframe::change_state_to(state_type new_state,
    state_modification_reason reason)
{
	using reason_t = state_modification_reason;
	if (state_data == new_state) return;

	switch (state_data) {
		using enum state_type;
	case modifying:
		if (reason == reason_t::needs_series_type
		    || reason == reason_t::needs_sorted_records)
			return;
		if (auto *owned = get_if<source_type::owning>(&source))
			(*owned)->normalize_sizes();
		break;
	case aggregating: migrate_data(); break;
	case sorting:
		if (reason == reason_t::needs_record_count
		    || reason == reason_t::needs_series_type)
			return;
		if (auto *ptr = std::get_if<copy_source>(&source)) {
			if (auto &&indices = ptr->other->get_sorted_indices(
			        std::exchange(unsafe_get<sorting>(state_data),
			            {}));
			    !std::ranges::is_sorted(indices))
				ptr->sorted_indices = std::move(indices);
		}
		else {
			auto &owning = *unsafe_get<source_type::owning>(source);
			if (auto &&indices = owning.get_sorted_indices(
			        std::exchange(unsafe_get<sorting>(state_data),
			            {}));
			    !std::ranges::is_sorted(indices))
				owning.sort(std::move(indices));
		}
		break;
	default:
	case finalized:
		if (reason != reason_t::needs_own_state) return;
		break;
	}

	switch (new_state) {
		using enum state_type;
	case modifying:
		if (state_data != aggregating) migrate_data();
		state_data.emplace<modifying>();
		break;
	case aggregating:
		if (auto *ptr = std::get_if<copy_source>(&source);
		    ptr && ptr->sorted_indices)
			error();
		state_data.emplace<aggregating>();
		break;
	case sorting:
		if (auto *ptr = std::get_if<copy_source>(&source);
		    ptr && ptr->sorted_indices)
			error();

		state_data.emplace<sorting>();
		break;
	case finalized: {
		if (auto *ptr = get_if<source_type::owning>(&source))
			(*ptr)->finalize();
		state_data.emplace<finalized>();
		break;
	}
	}
}

const data_source &dataframe::get_data_source() const
{
	using enum source_type;
	return source == owning ? *unsafe_get<owning>(source)
	                        : *unsafe_get<copying>(source).other;
}

std::string_view dataframe::get_series_info(
    const std::string_view &id,
    const char *key) const &
{
	switch (auto &&ser = get_data_source().get_series(id)) {
		using enum series_type;
	default: return {};
	case measure: {
		const auto &info = unsafe_get<measure>(ser).second.info;
		auto it = info.find(key);
		return it == info.end() ? std::string_view{} : it->second;
	}
	case dimension: {
		const auto &info = unsafe_get<dimension>(ser).second.info;
		auto it = info.find(key);
		return it == info.end() ? std::string_view{} : it->second;
	}
	}
}

}
