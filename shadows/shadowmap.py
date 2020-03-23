#!/usr/bin/env python3

"""
Class for computing which pixels of an elevation raster are lit and which are in
shade at a given time.
"""

# ---------------------------------------------------------------------------- #

from dataclasses import dataclass
from datetime import datetime
from math import cos, sin, radians
from numpy import float64, uint8
from pvlib.solarposition import get_solarposition
from PIL import Image
from sys import exit
from . import raster

try:
    import c_shadowmap
except ImportError:
    print('Cannot import extension "c_shadowmap".')
    print('Try executing "python setup.py install" before using this program.')
    exit(0)

# ---------------------------------------------------------------------------- #

@dataclass
class ShadowMap:
    """
    Compute which pixels of an elevation raster are lit and which are in shade
    at a given time.
    """

    dem: raster.Raster
    date_time: datetime = datetime.now()
    view_height: float64 = 0.0

    # ------------------------------------------------------------------------ #

    def __post_init__(self):
        """
        Compute the sun position in the sky for a given time and location.
        """
        sp = get_solarposition(self.date_time, self.dem.lat, self.dem.lng)
        self.altitude = radians(sp['elevation'])
        self.azimuth = radians(sp['azimuth'])
        self.sun_x = sin(self.azimuth) * cos(self.altitude)
        self.sun_y = -cos(self.azimuth) * cos(self.altitude)
        self.sun_z = sin(self.altitude) * self.dem.resolution

    # ------------------------------------------------------------------------ #

    def compute(self, data_type = uint8):
        """
        Compute which pixels of the elevation raster are lit or not.
        """
        return c_shadowmap.calculate(self.dem.elevation_map,
                                     self.sun_x, self.sun_y, self.sun_z,
                                     self.view_height,
                                     self.dem.max_elevation).astype(uint8)

    # ------------------------------------------------------------------------ #

    def to_image(self):
        """
        Convert to image the shadow map.
        """
        data = self.compute()
        rescaled = (255.0 / data.max() * (data - data.min())).astype(uint8)
        return Image.fromarray(rescaled)

# ---------------------------------------------------------------------------- #
