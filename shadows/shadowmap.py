#!/usr/bin/env python3

"""
Class for computing which pixels of an elevation raster are lit and which are in
shade at a given time.
"""

# ---------------------------------------------------------------------------- #

from dataclasses import dataclass
from datetime import datetime
from math import cos, sin, radians
from numpy import float32, float64, uint8, ndarray, full
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

    dsm: raster.Raster
    date_time: datetime = datetime.now()
    view_height: float64 = 0.0

    # ------------------------------------------------------------------------ #

    def __post_init__(self):
        """
        Compute the sun position in the sky for a given time and location.
        """
        sp = get_solarposition(self.date_time, self.dsm.lat, self.dsm.lng)
        self.altitude = radians(sp['elevation'])
        self.azimuth = radians(sp['azimuth'])
        self.sun_x = self.dsm.precision(sin(self.azimuth) * cos(self.altitude))
        self.sun_y = self.dsm.precision(-cos(self.azimuth) * cos(self.altitude))
        self.sun_z = self.dsm.precision(sin(self.altitude) * self.dsm.resolution)
        self.view_height = self.dsm.precision(self.view_height)

    # ------------------------------------------------------------------------ #

    def compute(self):
        """
        Compute which pixels of the elevation raster are lit or not.
        """
        if self.dsm.precision == float32:
            self.shadows = c_shadowmap.shadowmap_raster_f(
                                                   self.dsm.elevation_map,
                                                   self.sun_x, self.sun_y,
                                                   self.sun_z, self.view_height,
                                                   self.dsm.max_elevation)
        elif self.dsm.precision == float64:
            self.shadows = c_shadowmap.shadowmap_raster_d(
                                                   self.dsm.elevation_map,
                                                   self.sun_x, self.sun_y,
                                                   self.sun_z, self.view_height,
                                                   self.dsm.max_elevation)
        return self.shadows

    # ------------------------------------------------------------------------ #

    def to_image(self):
        """
        Convert to image the shadow map.
        """
        try:
            data = self.shadows
        except:
            data = self.compute()
        rescaled = (255.0 / data.max() * (data - data.min())).astype(uint8)
        return Image.fromarray(rescaled)

# ---------------------------------------------------------------------------- #

@dataclass
class ShadowIndex:
    """
    Compute which pixels of an elevation raster are lit and which are in shade
    at a given time.
    """

    dsm: raster.Raster
    row_idxs: ndarray
    col_idxs: ndarray
    sun_x: float64
    sun_y: float64
    sun_z: float64
    shade_value: uint8 = 0
    lit_value: uint8 = 254

    # ------------------------------------------------------------------------ #

    def compute(self):
        """
        Compute which pixels of the elevation raster are lit or not.
        """
        if self.dsm.precision == float32:
            self.shadows = c_shadowmap.shadowmap_indexes_f(
                                             self.dsm.elevation_map,
                                             self.row_idxs, self.col_idxs,
                                             self.sun_x, self.sun_y, self.sun_z,
                                             self.dsm.max_elevation,
                                             self.shade_value, self.lit_value)
        elif self.dsm.precision == float64:
            self.shadows = c_shadowmap.shadowmap_indexes_d(
                                             self.dsm.elevation_map,
                                             self.row_idxs, self.col_idxs,
                                             self.sun_x, self.sun_y, self.sun_z,
                                             self.dsm.max_elevation,
                                             self.shade_value, self.lit_value)

        return self.shadows

    # ------------------------------------------------------------------------ #

    def to_image(self):
        """
        Convert to image the shadow map.
        """
        try:
            data = self.shadows
        except:
            data = self.compute()
        S = full(self.dsm.elevation_map.shape, fill_value=128, dtype=uint8)
        S[self.row_idxs, self.col_idxs] = \
                        (255.0 / data.max() * (data - data.min())).astype(uint8)
        return Image.fromarray(S)

# ---------------------------------------------------------------------------- #
