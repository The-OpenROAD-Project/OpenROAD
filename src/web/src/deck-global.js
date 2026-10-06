// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The parts of deck.gl the layout view draws with, handed over as window.deck;
// the entries import this module first.  layout-view.js reads the global, so
// its unit tests can hand it a stand-in instead.

import { Deck, OrthographicView } from '@deck.gl/core';
import { PathStyleExtension } from '@deck.gl/extensions';
import { BitmapLayer, PathLayer, ScatterplotLayer, SolidPolygonLayer }
    from '@deck.gl/layers';

window.deck = {
    BitmapLayer,
    Deck,
    OrthographicView,
    PathLayer,
    PathStyleExtension,
    ScatterplotLayer,
    SolidPolygonLayer,
};
