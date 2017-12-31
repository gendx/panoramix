/*
    Panoramix - 3D view of your surroundings.
    Copyright (C) 2017  Guillaume Endignoux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/gpl-3.0.txt
*/

#ifndef CONFIG_HPP
#define CONFIG_HPP

// Folder for cached tiles.
static constexpr char CACHE_FOLDER[] = "data/";

// Filename for labels inside CACHE_FOLDER.
static constexpr char LABELS_FILE[] = "labels";

// Filename for index inside CACHE_FOLDER.
static constexpr char INDEX_FILE[] = "index";

// Max number of concurrent HTTPS requests.
static constexpr unsigned int MAX_REQUESTS = 10;

// Max number of tiles to keep in the cache (cf. https://www.mapbox.com/help/mobile-offline/).
static constexpr unsigned int CACHE_LIMIT = 5000;

// API token for Mapbox requests.
static constexpr char MAPBOX_TOKEN[] = "***";

// Domain for API requests.
static constexpr char MAPBOX_DOMAIN[] = "a.tiles.mapbox.com";

// Source for API tile requests.
static constexpr char MAPBOX_SOURCE[] = "mapbox.mapbox-terrain-v2";

// For some reason retina displays on MacOS have twice more pixel resolution
// than coordinates on screen.  For example, a window of dimensions 1000 x 500
// has 2000 x 1000 pixels.  This constant is here to correctly convert between
// screen coordinates and indices in pixel arrays.  Put "2" on retina displays
// and "1" on regular displays.
static constexpr int RETINA_FACTOR = 2;

// Meters above ground for an observer.
static constexpr double VIEWER_HEIGHT = 10;

// Meters above ground for depth test of peaks.
static constexpr double PEAKS_DEPTH_HEIGHT = 100;

// Maximum number of labels to show in one view.  No limit if undefined.
//#define MAX_LABELS_IN_VIEW 20

// If not defined, use a simpler model of locally flat Earth.
// Note: when enabled, the Earth is simply approximated as a sphere (instead of
// an ellipsoid).
#define USE_EARTH_CURVATURE

// Earth radius in meters.
static constexpr double EARTH_RADIUS = 6.384e6;

// Enable various assert()s in geometric algorithms.
#define ENABLE_GEOMETRIC_ASSERT

#endif // CONFIG_HPP
