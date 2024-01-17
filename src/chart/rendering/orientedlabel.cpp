#include "orientedlabel.h"

#include "base/geom/angle.h"

namespace Vizzu::Draw
{

OrientedLabel OrientedLabel::create(Gfx::ICanvas &canvas,
    const std::string &text,
    const Geom::Line &labelPos,
    const Styles::OrientedLabel &labelStyle,
    double centered)
{
	if (text.empty()) return {text};

	const Gfx::Font font(labelStyle);
	canvas.setFont(font);

	auto neededSize = Gfx::ICanvas::textBoundary(font, text);
	auto margin = labelStyle.toMargin(neededSize, font.size);
	auto paddedSize = neededSize + margin.getSpace();

	auto baseAngle = labelPos.getDirection().angle() + M_PI / 2.0;

	auto absAngle =
	    labelStyle.orientation->combine<double>(
	        [&](int, const auto &orientation)
	        {
		        using enum Styles::OrientedLabel::Orientation;
		        switch (orientation) {
		        default:
		        case horizontal: return 0.0;
		        case vertical: return M_PI / 2.0;
		        case normal: return labelPos.getDirection().angle();
		        case tangential:
			        return labelPos.getDirection().angle()
			             + M_PI / 2.0;
		        }
	        })
	    + labelStyle.angle->rad();

	auto relAngle = Geom::Angle(absAngle - baseAngle).rad();
	if (relAngle > M_PI) relAngle -= M_PI;

	auto xOffsetAngle = relAngle < M_PI / 4.0     ? 0
	                  : relAngle < 3 * M_PI / 4.0 ? M_PI / 2.0
	                                              : M_PI;

	auto offset = Geom::Point{-sin(relAngle + xOffsetAngle)
	                              * paddedSize.x / 2.0,
	                  -fabs(cos(relAngle)) * paddedSize.y / 2
	                      - sin(relAngle) * paddedSize.x / 2}
	            * (1 - centered) * labelPos.getDirection().abs();

	auto transform =
	    Geom::AffineTransform(labelPos.begin, 1.0, -baseAngle)
	    * Geom::AffineTransform(offset, 1.0, -relAngle)
	    * Geom::AffineTransform(paddedSize / -2, 1.0, 0);

	auto realAngle = Geom::Angle(baseAngle + relAngle).rad();
	auto upsideDown =
	    realAngle > M_PI / 2.0 && realAngle < 3 * M_PI / 2.0;

	if (upsideDown)
		transform =
		    transform * Geom::AffineTransform(paddedSize, 1.0, -M_PI);

	return {text,
	    {transform, {paddedSize}},
	    {margin.topLeft(), neededSize}};
}

void OrientedLabel::draw(Gfx::ICanvas &canvas,
    RenderedChart &renderedChart,
    const Gfx::Color &textColor,
    const Gfx::Color &bgColor,
    Util::EventDispatcher::Event &event,
    std::unique_ptr<Util::EventTarget> eventTarget) const
{
	if (!bgColor.isTransparent()) {
		canvas.save();
		canvas.setBrushColor(bgColor);
		canvas.setLineColor(bgColor);
		canvas.transform(rect.transform);
		canvas.rectangle(Geom::Rect(Geom::Point(), rect.size));
		canvas.restore();
	}

	if (!textColor.isTransparent()) {
		canvas.save();
		canvas.setTextColor(textColor);

		if (event.invoke(
		        Events::OnTextDrawEvent{*eventTarget, rect, text})) {
			canvas.transform(rect.transform);
			canvas.text(contentRect, text);
			renderedChart.emplace(rect, std::move(eventTarget));
		}
		canvas.restore();
	}
}

}