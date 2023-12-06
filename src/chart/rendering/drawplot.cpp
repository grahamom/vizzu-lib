#include "drawplot.h"

#include "base/text/smartstring.h"
#include "chart/rendering/drawaxes.h"
#include "chart/rendering/drawbackground.h"

#include "markerrenderer.h"

namespace Vizzu::Draw
{

void DrawPlot::draw(Gfx::ICanvas &canvas,
    Painter &painter,
    const Geom::Rect &plotRect) const
{
	DrawBackground{{ctx()}}.draw(canvas,
	    plotRect,
	    rootStyle.plot,
	    *rootEvents.draw.plot.background,
	    Events::Targets::plot());

	drawPlotArea(canvas, painter, false);

	auto axes = DrawAxes{{ctx()},
	    canvas,
	    painter,
	    {{ctx()}, canvas, painter}};
	axes.drawGeometries();

	auto clip = rootStyle.plot.overflow == Styles::Overflow::hidden;

	if (clip) {
		canvas.save();
		drawPlotArea(canvas, painter, true);
	}

	auto markerRenderer = MarkerRenderer{{ctx()}, canvas, painter};
	markerRenderer.drawLines();

	markerRenderer.drawMarkers();

	if (clip) canvas.restore();

	markerRenderer.drawLabels();

	axes.drawLabels();
}

void DrawPlot::drawPlotArea(Gfx::ICanvas &canvas,
    Painter &painter,
    bool clip) const
{
	auto areaElement = Events::Targets::area();

	auto rect = Geom::Rect::Ident();
	painter.setPolygonToCircleFactor(0.0);
	painter.setPolygonStraightFactor(0.0);
	painter.setResMode(ResolutionMode::High);

	if (clip) { painter.drawPolygon(rect.points(), true); }
	else {
		Events::OnRectDrawEvent eventObj(*areaElement, {rect, true});

		if (!rootStyle.plot.areaColor->isTransparent()) {
			canvas.setBrushColor(*rootStyle.plot.areaColor);
			canvas.setLineColor(*rootStyle.plot.areaColor);
			canvas.setLineWidth(0);
			if (rootEvents.draw.plot.area->invoke(
			        std::move(eventObj))) {
				painter.drawPolygon(rect.points(), false);
				renderedChart.emplace(Rect{rect, true},
				    std::move(areaElement));
			}
			canvas.setLineWidth(0);
		}
		else if (rootEvents.draw.plot.area->invoke(
		             std::move(eventObj))) {
			renderedChart.emplace(Rect{rect, true},
			    std::move(areaElement));
		}
	}
}

}