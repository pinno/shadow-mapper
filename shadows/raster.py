#!/usr/bin/env python3

"""
Class for preparing an elevation raster to be employed by the ShadowMap class.
"""

# ---------------------------------------------------------------------------- #

from dataclasses import dataclass
from numpy import amax, float64
import rasterio

# ---------------------------------------------------------------------------- #

@dataclass
class Raster:
    """
    Get raster data and metadata needed by the ShadowMap class.
    """

    filename: str

    # ------------------------------------------------------------------------ #

    def __post_init__(self):
        """
        Get raster data and metadata by means of Rasterio library.
        """

        with rasterio.open(self.filename, 'r+') as f:
            self.elevation_map = f.read(1, out_dtype=float)
            self.max_elevation = amax(self.elevation_map).astype(float64)
            # Check whether the CRS is included in the raster
            try:
                self.crs = f.crs.to_string()
            except AttributeError:
                print('Warning! Cannot find CRS for the current raster!')
                print('Setting raster CRS to "EPSG:32632".')
                f.crs = rasterio.crs.CRS({"init": "EPSG:32632"})
                self.crs = f.crs.to_string()
            # Longitude and latitude in EPSG:4326 of the center of the raster
            (self.lng, self.lat) = f.lnglat()
            # Size of the raster, in pixels
            self.width = f.width
            self.height = f.height
            # Meters per pixel
            self.resolution = f.count

# ---------------------------------------------------------------------------- #
