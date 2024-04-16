#include "datatable.h"

#include <base/conv/auto_json.h>
#include <base/text/funcstring.h>
#include <cmath>
#include <data/datacube/datacube.h>
#include <dataframe/impl/aggregators.h>
#include <dataframe/interface.h>
#include <numeric>
#include <utility>

#include "data/datacube/datacubeoptions.h"
#include "data/datacube/datafilter.h"
#include "data/datacube/seriesindex.h"
#include "data/table/datatable.h"

namespace Vizzu::Data
{

void data_table::addColumn(std::string_view name,
    std::string_view unit,
    const std::span<const double> &values)
{
	df.add_measure(values,
	    name.data(),
	    dataframe::adding_type::create_or_override,
	    {{std::pair{"unit", unit.data()}}});
}

void data_table::addColumn(std::string_view name,
    const std::span<const char *const> &categories,
    const std::span<const std::uint32_t> &values)
{
	df.add_dimension(categories,
	    values,
	    name.data(),
	    dataframe::adding_type::create_or_override,
	    {});

	df.remove_unused_categories(name);

	for (const auto &cat : df.get_categories(name))
		if (cat.empty()) {
			df.fill_na(name, "");
			break;
		}
}

void data_table::pushRow(const std::span<const char *> &cells)
{
	df.add_record(cells);

	for (const auto &dim : df.get_dimensions())
		for (const auto &cat : df.get_categories(dim))
			if (cat.empty()) {
				df.fill_na(dim, "");
				break;
			}
}

std::string data_table::getInfos() const { return df.as_string(); }

bool series_index_t::isDimension() const { return !aggr; }

const std::string_view &series_index_t::getColIndex() const
{
	return sid;
}

bool operator==(const series_index_t &lhs, const series_index_t &rhs)
{
	return lhs.sid == rhs.sid && lhs.aggr == rhs.aggr;
}

bool operator<(const series_index_t &lhs, const series_index_t &rhs)
{
	return lhs.orig_index < rhs.orig_index
	    || (lhs.orig_index == rhs.orig_index && lhs.aggr < rhs.aggr);
}

bool slice_index_t::operator<(slice_index_t const &rhs) const
{
	return orig_index < rhs.orig_index
	    || (orig_index == rhs.orig_index
	        && orig_value < rhs.orig_value);
}

bool slice_index_t::operator==(const slice_index_t &rhs) const
{
	return orig_index == rhs.orig_index
	    && orig_value == rhs.orig_value;
}

bool data_cube_t::multi_index_t::empty() const
{
	return parent->df->get_dimensions().empty();
}

bool data_cube_t::multi_index_t::isEmpty() const
{
	return !rid || parent->df->get_record_count() == 0
	    || parent->df->is_filtered(*rid);
}

bool data_cube_t::Id::operator==(const Id &id) const
{
	return itemSliceIndex == id.itemSliceIndex
	    && seriesId == id.seriesId && itemId == id.itemId;
}

data_cube_t::data_t::iterator_t::iterator_t(std::size_t rid,
    const data_t *parent,
    std::size_t old) :
    rid(rid),
    parent(parent),
    old(old)
{
	if (!old) incr();
}

void data_cube_t::data_t::iterator_t::incr()
{
	if (found) { ++rid; }

	auto &&indices = parent->get_indices(old);

	found = false;
	for (std::size_t ix{}; ix < indices.size(); ++ix) {
		auto &&dim = parent->dim_reindex[ix];
		auto &&cats = parent->df->get_categories(dim);
		auto val = std::get<std::string_view>(
		    parent->df->get_data(rid, dim));
		if (cats.size() == indices[ix]) {
			if (val.data() != nullptr) return;
		}
		else if (cats[indices[ix]] != val)
			return;
	}
	found = true;
}

bool data_cube_t::data_t::iterator_t::operator!=(
    const iterator_t &oth) const
{
	return old != oth.old;
}

data_cube_t::data_t::iterator_t &
data_cube_t::data_t::iterator_t::operator++()
{
	++old;

	incr();

	return *this;
}

data_cube_t::multi_index_t
data_cube_t::data_t::iterator_t::operator*() const
{
	return {parent,
	    found ? std::make_optional(rid) : std::nullopt,
	    parent->get_indices(old)};
}

series_index_t::series_index_t(std::string const &str,
    const data_table &table) :
    orig_name(str)
{
	constinit static auto names =
	    Refl::get_names<dataframe::aggregator_type>();
	if (const Text::FuncString func(str, false);
	    !func.isEmpty()
	    && std::find(names.begin(), names.end(), func.getName())
	           != names.end()) {
		aggr = Refl::get_enum<dataframe::aggregator_type>(
		    func.getName());
		if (!func.getParams().empty()) {
			auto &&[s, i, type] =
			    table.getDf().get_series_meta(func.getParams().at(0));
			sid = s;
			orig_index = i;
		}
	}
	else {
		auto &&[s, i, type] = table.getDf().get_series_meta(str);
		sid = s;
		orig_index = i;
		if (type == DataTable::Type::measure)
			aggr = dataframe::aggregator_type::sum;
	}
}

std::vector<std::size_t> data_cube_t::data_t::get_indices(
    std::size_t ix) const
{
	auto res = sizes;
	for (auto &cur : std::ranges::views::reverse(res)) {
		if (!cur) continue;
		auto &&newCur = ix % cur;
		ix /= cur;
		cur = newCur;
	}
	return res;
}

data_cube_t::data_t::iterator_t data_cube_t::data_t::begin() const
{
	return {std::size_t{}, this, std::size_t{}};
}

data_cube_t::data_t::iterator_t data_cube_t::data_t::end() const
{
	return {df->get_record_count(), this, full_size};
}

template <>
data_cube_t::data_cube_t(const data_table &table,
    const DataCubeOptions &options,
    const Filter &filter) :
    table(&table),
    df(options.getDimensions().empty()
                && options.getMeasures().empty()
            ? dataframe::dataframe::create_new()
            : table.getDf().copy(false)),
    data{df.get(), options}
{
	df->remove_records(filter.getFunction());

	removed = df->copy(false);

	for (const auto &dim : options.getDimensions())
		df->aggregate_by(dim.getColIndex());

	for (const auto &meas : options.getMeasures()) {
		auto &&sid = meas.getColIndex();
		auto &&aggr = meas.getAggr();

		measure_names.try_emplace(std::pair{sid, aggr},
		    df->set_aggregate(sid, aggr));
	}

	if (options.getMeasures().empty()
	    && !options.getDimensions().empty()) {
		auto &&unkn = std::string_view{};
		auto &&aggr = dataframe::aggregator_type::exists;
		measure_names.try_emplace(std::pair{unkn, aggr},
		    df->set_aggregate(unkn, aggr));
	}

	for (const auto &dim : options.getDimensions()) {
		df->set_sort(dim.getColIndex(),
		    dataframe::sort_type::by_categories,
		    dataframe::na_position::last);
	}

	df->finalize();

	auto it = data.dim_reindex.begin();
	for (auto sizIt = data.sizes.begin();
	     const auto &dim : options.getDimensions()) {
		auto &&dimName = dim.getColIndex();
		*it++ = *std::lower_bound(df->get_dimensions().begin(),
		    df->get_dimensions().end(),
		    dimName);
		*sizIt++ =
		    df->get_categories(dimName).size() + df->has_na(dimName);
	}

	if (!options.getMeasures().empty()
	    || !options.getDimensions().empty())
		data.full_size = std::reduce(data.sizes.begin(),
		    data.sizes.end(),
		    std::size_t{1},
		    std::multiplies{});
}

size_t data_cube_t::combinedSizeOf(
    const series_index_list_t &colIndices) const
{
	std::size_t my_res{1};
	for (const auto &si : colIndices) {
		auto sid = si.getColIndex();
		my_res *= df->get_categories(sid).size() + df->has_na(sid);
	}
	return my_res;
}

size_t data_cube_t::subCellSize() const
{
	return df->get_measures().size();
}

std::string data_cube_t::getValue(const slice_index_t &index,
    std::string &&def) const
{
	auto my_res = index.value;
	if (my_res.data() == nullptr) {
		if (df->has_na(index.column)) { my_res = ""; }
		else {
			my_res = def;
		}
	}
	return std::string{my_res};
}

const std::string &data_cube_t::getName(
    const series_index_t &seriesId) const
{
	return measure_names.at(
	    {seriesId.getColIndex(), seriesId.getAggr()});
}

data_cube_t::Id data_cube_t::getId(const series_index_list_t &sl,
    const multi_index_t &mi) const
{
	std::map<std::string_view, std::size_t> reindex;
	struct DimProp
	{
		std::string_view value;
		std::size_t index;
		std::size_t size;
		std::size_t orig_ix;
	};
	std::vector<std::pair<std::string_view, DimProp>> v;
	for (std::size_t ix{}; const auto &s : sl)
		reindex[v.emplace_back(s.getColIndex(), DimProp{}).first] =
		    ix++;

	std::size_t seriesId{};
	for (std::size_t ix{}; ix < mi.parent->dim_reindex.size(); ++ix) {
		auto &&name = mi.parent->dim_reindex[ix];
		if (auto it = reindex.find(name); it != reindex.end()) {
			auto &&cats = mi.parent->df->get_categories(name);
			auto &[cat, cix, size, orig_ix] = v[it->second].second;
			orig_ix = ix;
			cix = mi.old[ix];
			size = cats.size() + mi.parent->df->has_na(name);
			if (cix < cats.size()) cat = cats[cix];
		}
		else {
			seriesId *=
			    df->get_categories(name).size() + df->has_na(name);
			seriesId += mi.old[ix];
		}
	}

	Id::SubSliceIndex ssi;
	std::size_t itemId{};
	for (std::size_t i{}; i < v.size(); ++i) {
		auto &[cat, cix, size, orig_ix] = v[i].second;

		ssi.emplace_back(v[i].first, cat, orig_ix, cix);
		itemId *= size;
		itemId += cix;
	}

	return {mi, ssi, seriesId, itemId};
}

data_cube_t::CellInfo data_cube_t::cellInfo(
    const multi_index_t &index) const
{
	CellInfo my_res;

	for (std::size_t ix{}; ix < index.parent->dim_reindex.size();
	     ++ix) {
		auto &&name = index.parent->dim_reindex[ix];
		auto cats = index.parent->df->get_categories(name);
		auto cix = index.old[ix];
		my_res.categories[name] =
		    cix < cats.size() ? cats[cix] : std::string_view{};
	}

	if (index.rid)
		for (auto &&meas : df->get_measures()) {
			if (meas == "exists()") continue;
			my_res.values[meas] = std::get<double>(
			    index.parent->df->get_data(*index.rid, meas));
		}
	else
		for (auto &&meas : df->get_measures()) {
			if (meas == "exists()") continue;
			my_res.values[meas] = 0.0;
		}

	return my_res;
}

double data_cube_t::valueAt(const multi_index_t &multiIndex,
    const series_index_t &seriesId) const
{
	if (multiIndex.rid) {
		const auto &name = getName(seriesId);
		return std::get<double>(df->get_data(*multiIndex.rid, name));
	}

	return {};
}

double data_cube_t::aggregateAt(const multi_index_t &multiIndex,
    const series_index_list_t &sumCols,
    const series_index_t &seriesId) const
{
	if (sumCols.empty()) return valueAt(multiIndex, seriesId);

	std::string id{};
	for (const auto &s : sumCols) {
		id += s.getColIndex();
		id += ';';
	}

	const auto &name = getName(seriesId);

	id += name;

	auto it = cacheImpl->find(id);
	if (it == cacheImpl->end()) {
		it = cacheImpl->emplace(id, removed->copy(false)).first;

		auto &cp = it->second;

		auto keep_this =
		    std::set<std::string_view>(df->get_dimensions().begin(),
		        df->get_dimensions().end());
		for (const auto &dim : sumCols)
			keep_this.erase(dim.getColIndex());

		for (const auto &dim : keep_this) cp->aggregate_by(dim);

		[[maybe_unused]] auto &&new_name =
		    cp->set_aggregate(seriesId.getColIndex(),
		        seriesId.getAggr());

		for (auto &&dim : keep_this)
			cp->set_sort(dim,
			    dataframe::sort_type::by_categories,
			    dataframe::na_position::last);

		cp->finalize();
	}

	auto &sub_df = it->second;

	if (multiIndex.rid) {
		auto &&rrid = df->get_record_id_by_dims(*multiIndex.rid,
		    sub_df->get_dimensions());
		return std::get<double>(sub_df->get_data(rrid, name));
	}

	// hack for sunburst.
	std::map<std::string_view, std::size_t> index;

	for (std::size_t ix{}; ix < multiIndex.parent->dim_reindex.size();
	     ++ix)
		index.emplace(multiIndex.parent->dim_reindex[ix],
		    multiIndex.old[ix]);

	std::string res;
	for (auto iit = index.begin();
	     auto &&dim : sub_df->get_dimensions()) {
		if (iit->first != dim) ++iit;

		auto &&cats = multiIndex.parent->df->get_categories(dim);
		res += dim;
		res += ':';
		res += iit->second == cats.size() ? "__null__"
		                                  : cats[iit->second];
		res += ';';
	}
	auto nanres = std::get<double>(sub_df->get_data(res, name));
	return std::isnan(nanres) ? double{} : nanres;
}

} // namespace Vizzu::Data
