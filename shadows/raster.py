#!/usr/bin/env python3

"""
Class for preparing an elevation raster to be employed by the ShadowMap class.
"""

# ---------------------------------------------------------------------------- #

from dataclasses import dataclass
from numpy import amax, float32, float64
from os.path import exists
from sys import exit
import rasterio

# ---------------------------------------------------------------------------- #

@dataclass
class Raster:
    """
    Get raster data and metadata needed by the ShadowMap class.
    """

    filename: str
    precision: type = float64

    # ------------------------------------------------------------------------ #

    def __post_init__(self):
        """
        Get raster data and metadata by means of Rasterio library.
        """

        if not exists(self.filename):
            print(f'Error! File {self.filename} does not exist!')
            exit(1)

        if self.precision not in [float32, float64]:
            print(f'Error! Precision must be either "numpy.float32" or "numpy.float64"!')
            exit(1)

        with rasterio.open(self.filename, 'r') as f:
            self.elevation_map = f.read(1, out_dtype=self.precision)
            self.max_elevation = amax(self.elevation_map).astype(self.precision)
            # Check whether the CRS is included in the raster
            try:
                self.crs = f.crs.to_string()
            except AttributeError:
                print('Error! Cannot find CRS for the current raster!')
                exit(1)
            # Longitude and latitude in EPSG:4326 of the center of the raster
            (self.lng, self.lat) = f.lnglat()
            # Size of the raster, in pixels
            self.width = f.width
            self.height = f.height
            # Meters per pixel
            self.resolution = f.count


# ---------------------------------------------------------------------------- #
