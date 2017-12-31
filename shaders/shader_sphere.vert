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

uniform highp mat4 matrix;
attribute highp vec4 vertex;
attribute highp vec4 normal;
varying highp vec4 fragNormal;
varying mediump vec4 fragColor;

void main(void)
{
    float t = vertex.w / 5000.0;
    if (t < 0.0)
        t = 0.0;
    else if (t > 2.0)
        t = 2.0;

    if (t < 1.0) {
        float u = 1.0 - t;
        fragColor = vec4(0.0, 0.8, 0.0, 1.0)*u + vec4(0.2, 0.2, 1.0, 1.0)*t;
    } else {
        t -= 1.0;
        float u = 1.0 - t;
        fragColor = vec4(0.2, 0.2, 1.0, 1.0)*u + vec4(0.8, 0.0, 0.0, 1.0)*t;
    }

    gl_Position = matrix * vec4(vertex.xyz, 1.0);
    fragNormal = normal;
}
