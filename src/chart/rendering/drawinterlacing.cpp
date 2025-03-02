#include "drawinterlacing.h"

#include "base/math/renard.h"
#include "base/text/smartstring.h"
#include "chart/rendering/drawlabel.h"
#include "chart/rendering/orientedlabel.h"

namespace Vizzu::Draw
{

DrawInterlacing::DrawInterlacing(const DrawingContext &context,
    bool text) :
    DrawingContext(context)
{
	draw(true, text);
	draw(false, text);
}

void DrawInterlacing::draw(bool horizontal, bool text)
{
	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;

	auto interlacingColor =
	    *rootStyle.plot.getAxis(axisIndex).interlacing.color;

	if (!text && interlacingColor.alpha <= 0.0) return;

	const auto &axis = plot.measureAxises.at(axisIndex);

	if (!axis.range.isReal()) return;

	auto enabled = axis.enabled.calculate<double>();

	auto step = axis.step.calculate();

	auto stepHigh = Math::Renard::R5().ceil(step);
	stepHigh = std::min(axis.step.max(),
	    std::max(stepHigh, axis.step.min()));

	auto stepLow = Math::Renard::R5().floor(step);
	stepLow =
	    std::min(axis.step.max(), std::max(stepLow, axis.step.min()));

	if (stepHigh == step) {
		draw(axis.enabled,
		    horizontal,
		    stepHigh,
		    enabled,
		    axis.range.size(),
		    text);
	}
	else if (stepLow == step) {
		draw(axis.enabled,
		    horizontal,
		    stepLow,
		    enabled,
		    axis.range.size(),
		    text);
	}
	else {
		auto highWeight =
		    Math::Range(stepLow, stepHigh).rescale(step) * enabled;

		auto lowWeight = (1.0 - highWeight) * enabled;

		draw(axis.enabled,
		    horizontal,
		    stepLow,
		    lowWeight,
		    axis.range.size(),
		    text);
		draw(axis.enabled,
		    horizontal,
		    stepHigh,
		    highWeight,
		    axis.range.size(),
		    text);
	}
}

void DrawInterlacing::draw(
    const ::Anim::Interpolated<bool> &axisEnabled,
    bool horizontal,
    double stepSize,
    double weight,
    double rangeSize,
    bool text)
{
	const auto &enabled = horizontal ? plot.guides.y : plot.guides.x;

	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;

	const auto &axisStyle = rootStyle.plot.getAxis(axisIndex);

	const auto &axis = plot.measureAxises.at(axisIndex);

	const auto origo = plot.measureAxises.origo();

	if (static_cast<double>(enabled.interlacings || enabled.axisSticks
	                        || enabled.labels)
	    > 0) {
		auto interlaceIntensity =
		    weight * static_cast<double>(enabled.interlacings);
		auto interlaceColor =
		    *axisStyle.interlacing.color * interlaceIntensity;

		auto tickIntensity =
		    weight * static_cast<double>(enabled.axisSticks);

		auto textAlpha = weight * static_cast<double>(enabled.labels);
		auto textColor = *axisStyle.label.color * textAlpha;

		if (rangeSize <= 0) return;

		auto stripWidth = stepSize / rangeSize;

		auto axisBottom = axis.origo() + stripWidth;

		auto iMin = axisBottom > 0 ? static_cast<int>(
		                std::floor(-axis.origo() / (2 * stripWidth)))
		                           : 0;

		if (stripWidth <= 0) return;
		auto interlaceCount = 0U;
		const auto maxInterlaceCount = 1000U;
		for (int i = iMin; ++interlaceCount <= maxInterlaceCount;
		     ++i) {
			auto bottom = axisBottom + i * 2 * stripWidth;
			if (bottom >= 1.0) break;
			auto clippedBottom = bottom;
			auto top = bottom + stripWidth;
			auto clipTop = top > 1.0;
			auto clipBottom = bottom < 0.0;
			auto topUnderflow = top <= 0.0;
			if (clipTop) top = 1.0;
			if (clipBottom) clippedBottom = 0.0;

			if (!topUnderflow) {
				Geom::Rect rect(Geom::Point{clippedBottom, 0.0},
				    Geom::Point{top - clippedBottom, 1.0});

				if (horizontal)
					rect =
					    Geom::Rect(rect.pos.flip(), rect.size.flip());

				if (text) {
					canvas.setTextColor(textColor);
					canvas.setFont(Gfx::Font{axisStyle.label});

					if (!clipBottom) {
						auto value = (i * 2 + 1) * stepSize;
						auto tickPos =
						    rect.bottomLeft().comp(!horizontal)
						    + origo.comp(horizontal);

						if (textColor.alpha > 0)
							drawDataLabel(axisEnabled,
							    horizontal,
							    tickPos,
							    value,
							    axis.unit,
							    textColor);

						if (tickIntensity > 0)
							drawSticks(tickIntensity,
							    horizontal,
							    tickPos);
					}
					if (!clipTop) {
						auto value = (i * 2 + 2) * stepSize;
						auto tickPos =
						    rect.topRight().comp(!horizontal)
						    + origo.comp(horizontal);

						if (textColor.alpha > 0)
							drawDataLabel(axisEnabled,
							    horizontal,
							    tickPos,
							    value,
							    axis.unit,
							    textColor);

						if (tickIntensity > 0)
							drawSticks(tickIntensity,
							    horizontal,
							    tickPos);
					}
				}
				else {
					canvas.save();

					canvas.setLineColor(Gfx::Color::Transparent());
					canvas.setBrushColor(interlaceColor);

					painter.setPolygonToCircleFactor(0);
					painter.setPolygonStraightFactor(0);

					auto eventTarget = std::make_unique<
					    Events::Targets::AxisInterlacing>(
					    !horizontal);

					if (rootEvents.draw.plot.axis.interlacing->invoke(
					        Events::OnRectDrawEvent(*eventTarget,
					            {rect, true}))) {
						painter.drawPolygon(rect.points());
						renderedChart.emplace(Draw::Rect{rect, true},
						    std::move(eventTarget));
					}

					canvas.restore();
				}
			}
		}
	}
}

void DrawInterlacing::drawDataLabel(
    const ::Anim::Interpolated<bool> &axisEnabled,
    bool horizontal,
    const Geom::Point &tickPos,
    double value,
    const std::string &unit,
    const Gfx::Color &textColor)
{
	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;
	const auto &labelStyle = rootStyle.plot.getAxis(axisIndex).label;

	auto str = Text::SmartString::fromNumber(value,
	    *labelStyle.numberFormat,
	    static_cast<size_t>(*labelStyle.maxFractionDigits),
	    *labelStyle.numberScale);

	if (!unit.empty()) {
		if (*labelStyle.numberFormat != Text::NumberFormat::prefixed)
			str += " ";

		str += unit;
	}

	auto normal = Geom::Point::Ident(horizontal);

	typedef Styles::AxisLabel::Position Pos;
	labelStyle.position->visit(
	    [this,
	        &labelStyle,
	        &axisEnabled,
	        &tickPos,
	        &horizontal,
	        &normal,
	        &str,
	        &textColor](int index, const auto &position)
	    {
		    if (labelStyle.position->interpolates()
		        && !axisEnabled.get(index).value)
			    return;

		    Geom::Point refPos = tickPos;

		    if (position.value == Pos::min_edge)
			    refPos[horizontal ? 0 : 1] = 0.0;

		    else if (position.value == Pos::max_edge)
			    refPos[horizontal ? 0 : 1] = 1.0;

		    auto under = labelStyle.position->interpolates()
		                   ? labelStyle.side->get(index).value
		                         == Styles::AxisLabel::Side::negative
		                   : labelStyle.side->factor<double>(
		                       Styles::AxisLabel::Side::negative);

		    auto sign = 1 - 2 * under;

		    auto posDir = coordSys.convertDirectionAt(
		        Geom::Line(refPos, refPos + normal));

		    posDir = posDir.extend(sign);

		    OrientedLabelRenderer labelRenderer(*this);
		    auto label =
		        labelRenderer.create(str, posDir, labelStyle, 0);
		    labelRenderer.render(label,
		        textColor * position.weight,
		        *labelStyle.backgroundColor,
		        rootEvents.draw.plot.axis.label,
		        std::make_unique<Events::Targets::AxisLabel>(str,
		            !horizontal));
	    });
}

void DrawInterlacing::drawSticks(double tickIntensity,
    bool horizontal,
    const Geom::Point &tickPos)
{
	auto axisIndex =
	    horizontal ? Gen::ChannelId::y : Gen::ChannelId::x;
	const auto &axisStyle = rootStyle.plot.getAxis(axisIndex);
	const auto &tickStyle = axisStyle.ticks;

	auto tickLength = tickStyle.length->get(
	    coordSys.getRect().size.getCoord(horizontal),
	    axisStyle.label.calculatedSize());

	if (tickStyle.color->isTransparent() || tickLength == 0
	    || *tickStyle.lineWidth == 0)
		return;

	auto tickColor = *tickStyle.color * tickIntensity;

	canvas.save();

	canvas.setLineColor(tickColor);
	canvas.setBrushColor(tickColor);

	auto direction =
	    horizontal ? Geom::Point::X(-1) : Geom::Point::Y(-1);

	auto tickLine = coordSys.convertDirectionAt(
	    Geom::Line(tickPos, tickPos + direction));

	tickLine = tickLine.segment(0, tickLength);

	if (*tickStyle.lineWidth > 0)
		canvas.setLineWidth(*tickStyle.lineWidth);

	typedef Styles::Tick::Position Pos;
	tickLine = tickStyle.position->combine<Geom::Line>(
	    [&](int, const auto &position)
	    {
		    switch (position) {
		    default:
		    case Pos::outside: return tickLine;
		    case Pos::inside: return tickLine.segment(-1, 0);
		    case Pos::center: return tickLine.segment(-0.5, 0.5);
		    }
	    });

	auto eventTarget =
	    std::make_unique<Events::Targets::AxisTick>(!horizontal);

	if (rootEvents.draw.plot.axis.tick->invoke(
	        Events::OnLineDrawEvent(*eventTarget,
	            {tickLine, false}))) {
		canvas.line(tickLine);
		renderedChart.emplace(Draw::Line{tickLine, false},
		    std::move(eventTarget));
	}
	if (*tickStyle.lineWidth > 1) canvas.setLineWidth(0);

	canvas.restore();
}

}