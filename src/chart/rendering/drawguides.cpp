#include "drawguides.h"

namespace Vizzu::Draw
{

DrawGuides::DrawGuides(const DrawingContext &context) :
    DrawingContext(context)
{
	draw(true);
	draw(false);
}

void DrawGuides::draw(bool horizontal)
{
	auto axisId = horizontal ? Gen::ChannelId::x : Gen::ChannelId::y;

	const auto &guideStyle = rootStyle.plot.getAxis(axisId).guides;

	auto baseColor = *guideStyle.color;
	if (baseColor.alpha == 0) return;

	const auto &axises = plot.dimensionAxises;
	const auto &axis = axises.at(axisId);

	if (axis.enabled && *guideStyle.lineWidth > 0
	    && (static_cast<double>(
	            plot.guides.at(axisId).dimensionGuides)
	        > 0)) {
		canvas.setLineWidth(*guideStyle.lineWidth);

		for (auto it = axis.begin(); it != axis.end(); ++it) {
			auto weight = it->second.weight;
			weight *= static_cast<double>(
			    plot.guides.at(axisId).dimensionGuides);
			if (weight == 0) continue;

			auto next = std::next(it);
			if (next != axis.end()) {
				drawGuide(horizontal,
				    it->second.range.getMax(),
				    baseColor * weight);
			}
		}

		canvas.setLineWidth(0);
	}
}

void DrawGuides::drawGuide(bool horizontal,
    double val,
    const Gfx::Color &color)
{
	auto eventTarget =
	    std::make_unique<Events::Targets::AxisGuide>(horizontal);

	auto ident = Geom::Point::Ident(horizontal);
	auto normal = Geom::Point::Ident(!horizontal);
	auto relMax = ident * val;

	canvas.setLineColor(color);
	const Geom::Line line(relMax, relMax + normal);
	if (rootEvents.draw.plot.axis.guide->invoke(
	        Events::OnLineDrawEvent(*eventTarget, {line, true}))) {
		painter.drawLine(line);
		renderedChart.emplace(Draw::Line{line, true},
		    std::move(eventTarget));
	}
}

}