Edge* clipLine(GPoint p0, GPoint p1, GRect bounds, Edge* edge) {
    if (p0.y() > p1.y()) {
        std::swap(p0, p1);
    }

    // out of range
    if (p0.y() <= bounds.top() && p1.y() <= bounds.top() || p0.y() >= bounds.bottom() && p1.y() >= bounds.bottom() || GRoundToInt(p0.y()) == GRoundToInt(p1.y())) {
        return edge;
    }

    // clip top 
    if (p0.y() < bounds.top()) {
        float newX = p0.x() + (p1.x() - p0.x()) * (bounds.top() - p0.y()) / (p1.y() - p0.y());
        p0.fX = newX;
        p0.fY = bounds.top();
    }

    // clip bottom 
    if (p1.y() > bounds.bottom()) {
        float newX = p1.x() - (p1.x() - p0.x()) * (p1.y() - bounds.bottom()) / (p1.y() - p0.y());
        p1.fX = newX;
        p1.fY = bounds.bottom();
    }

    if (p0.x() > p1.x()) {
        std::swap(p0, p1);
    }

    // outside left edge
    if (p1.x() <= bounds.left()) {
        p0.fX = p1.fX = bounds.left();
        return edge + edge->init(p0, p1);
    }

    // outside right edge
    if (p0.x() >= bounds.right()) {
        p0.fX = p1.fX = bounds.right();
        return edge + edge->init(p0, p1);
    }

    // edge projection left 
    if (p0.x() < bounds.left()) {
        float newY = p0.y() + (bounds.left() - p0.x()) * (p1.y() - p0.y()) / (p1.x() - p0.x());
        GPoint left = GPoint();
        left.fX = bounds.left();
        left.fY = p0.y();
        GPoint right = GPoint();
        right.fX = bounds.left();
        right.fY = newY;

        edge += edge->init(left, right);
        p0.fX = bounds.left();
        p0.fY = newY;
    }

    //edge projection right
    if (p1.x() > bounds.right()) {
        float newY = p1.y() - (p1.x() - bounds.right()) * (p1.y() - p0.y()) / (p1.x() - p0.x());
        GPoint left = GPoint();
        left.fX = bounds.right();
        left.fY = newY;
        GPoint right = GPoint();
        right.fX = bounds.right();
        right.fY = p1.y();

        edge += edge->init(left, right);

        p1.fX = bounds.right();
        p1.fY = newY;
    }

    return edge + edge->init(p0, p1);
}

