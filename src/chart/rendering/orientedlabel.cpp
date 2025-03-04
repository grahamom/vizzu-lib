#include "orientedlabel.h"

#include "base/geom/angle.h"

namespace Vizzu::Draw
{

OrientedLabelRenderer::OrientedLabelRenderer(
    const DrawingContext &context) :
    DrawingContext(context)
{}

OrientedLabel OrientedLabelRenderer::create(const std::string &text,
    const Geom::Line &labelPos,
    const Styles::OrientedLabel &labelStyle,
    double centered) const
{
	OrientedLabel res(text);

	if (text.empty()) return res;

	const Gfx::Font font(labelStyle);
	canvas.setFont(font);

	auto neededSize = canvas.textBoundary(text);
	auto margin = labelStyle.toMargin(neededSize, font.size);
	auto paddedSize = neededSize + margin.getSpace();

	auto baseAngle = labelPos.getDirection().angle() + M_PI / 2.0;

	typedef Styles::OrientedLabel::Orientation Ori;
	auto absAngle =
	    labelStyle.orientation->combine<double>(
	        [&](int, const auto &orientation)
	        {
		        switch (orientation) {
		        default:
		        case Ori::horizontal: return 0.0;
		        case Ori::vertical: return M_PI / 2.0;
		        case Ori::normal:
			        return labelPos.getDirection().angle();
		        case Ori::tangential:
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

	res.rect.transform = transform;
	res.rect.size = Geom::Size{paddedSize};

	res.contentRect = Geom::Rect(margin.topLeft(), neededSize);

	return res;
}

void OrientedLabelRenderer::render(const OrientedLabel &label,
    const Gfx::Color &textColor,
    const Gfx::Color &bgColor,
    const Util::EventDispatcher::event_ptr &event,
    std::unique_ptr<Util::EventTarget> eventTarget)
{
	if (!bgColor.isTransparent()) {
		canvas.save();
		canvas.setBrushColor(bgColor);
		canvas.setLineColor(bgColor);
		canvas.transform(label.rect.transform);
		canvas.rectangle(Geom::Rect(Geom::Point(), label.rect.size));
		canvas.restore();
	}

	if (!textColor.isTransparent()) {
		canvas.save();
		canvas.setTextColor(textColor);

		Events::Events::OnTextDrawEvent eventObj(*eventTarget,
		    label.rect,
		    label.text);

		if (event->invoke(std::move(eventObj))) {
			canvas.transform(label.rect.transform);
			canvas.text(label.contentRect, label.text);
			renderedChart.emplace(label.rect, std::move(eventTarget));
		}
		canvas.restore();
	}
}

}