#!/usr/bin/env python3

"""
Class for computing which pixels of an elevation raster are lit and which are in
shade at a given time.
"""

# ---------------------------------------------------------------------------- #

import astropy.coordinates as coords
import astropy.units as u
from astropy.time import Time
from dataclasses import dataclass
from datetime import datetime
from math import cos, sin
from numpy import float64, uint8
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
        location_coords = coords.EarthLocation(lon=self.dem.lng * u.deg,
                                               lat=self.dem.lat * u.deg)
        location_time = Time(self.date_time)
        angles = coords.AltAz(location=location_coords, obstime=location_time)
        sun = coords.get_sun(location_time)
        sun_altitude = sun.transform_to(angles).alt.radian
        sun_azimuth = sun.transform_to(angles).az.radian
        self.sun_x = sin(sun_azimuth) * cos(sun_altitude)
        self.sun_y = -cos(sun_azimuth) * cos(sun_altitude)
        self.sun_z = sin(sun_altitude) * self.dem.resolution

    # ------------------------------------------------------------------------ #

    def compute(self):
        """
        Compute which pixels of the elevation raster are lit or not.
        """
        return c_shadowmap.calculate(self.dem.elevation_map,
                                     self.sun_x, self.sun_y, self.sun_z,
                                     self.view_height, self.dem.max_elevation)

    # ------------------------------------------------------------------------ #

    def to_image(self):
        """
        Convert to image the shadow map.
        """
        data = self.compute()
        rescaled = (255.0 / data.max() * (data - data.min())).astype(uint8)
        return Image.fromarray(rescaled)

# ---------------------------------------------------------------------------- #
