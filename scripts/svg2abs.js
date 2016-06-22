function convertToAbsolute(path) {
  var x0, y0, x1, y1, x2, y2, segs = path.pathSegList;

  for (var x = 0, y = 0, i = 0, len = segs.numberOfItems; i < len; i++) {
    var seg = segs.getItem(i), c = seg.pathSegTypeAsLetter;

    if (/[MLHVCSQTA]/.test(c)){
      if ('x' in seg) x = seg.x;
      if ('y' in seg) y = seg.y;

    } else {
      if ('x1' in seg) x1 = x + seg.x1;
      if ('x2' in seg) x2 = x + seg.x2;
      if ('y1' in seg) y1 = y + seg.y1;
      if ('y2' in seg) y2 = y + seg.y2;
      if ('x'  in seg) x += seg.x;
      if ('y'  in seg) y += seg.y;

      switch(c) {
      case 'm':
        segs.replaceItem(path.createSVGPathSegMovetoAbs(x, y), i);
        break;
      case 'l':
        segs.replaceItem(path.createSVGPathSegLinetoAbs(x, y), i);
        break;
      case 'h':
        segs.replaceItem(path.createSVGPathSegLinetoHorizontalAbs(x), i);
        break;
      case 'v':
        segs.replaceItem(path.createSVGPathSegLinetoVerticalAbs(y), i);
        break;
      case 'c':
        segs.replaceItem(
          path.createSVGPathSegCurvetoCubicAbs(x, y, x1, y1, x2, y2), i);
        break;
      case 's':
        segs.replaceItem(
          path.createSVGPathSegCurvetoCubicSmoothAbs(x, y, x2, y2), i);
        break;
      case 'q':
        segs.replaceItem(
          path.createSVGPathSegCurvetoQuadraticAbs(x, y, x1, y1), i);
        break;
      case 't':
        segs.replaceItem(
          path.createSVGPathSegCurvetoQuadraticSmoothAbs(x, y), i);
        break;
      case 'a':
        segs.replaceItem(
          path.createSVGPathSegArcAbs(x, y, seg.r1, seg.r2, seg.angle,
                                      seg.largeArcFlag, seg.sweepFlag), i);
        break;

      case 'z': case 'Z':
        x = x0;
        y = y0;
        break;
      }
    }

    // Record the start of a subpath
    if (c == 'M' || c == 'm') x0 = x, y0 = y;
  }
}
