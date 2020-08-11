
# Shadow Mapper

This software is a lightweight version of [shadow-mapper](https://github.com/perliedman/shadow-mapper): given an elevation raster, it computes which parts of the geographic area are lit by the sun and which are in shade at a certain time.

Single or double precision can be chosen when instantiating the Raster object: results and execution times are supposed to stay the same (with very minor differences), while on the other hand the single precision allows for saving a lot of memory when analyzing huge areas.


## Installation

It works with [Python 3.7](https://www.python.org/). The following libraries are needed:

* [Pillow (7.0.0)](https://python-pillow.org/)
* [numpy (1.18.1)](https://numpy.org/)
* [pvlib (0.7.1)](https://pvlib-python.readthedocs.io/)
* [rasterio (1.1.1)](https://rasterio.readthedocs.io/)

Additional libraries are needed for running the below examples [2](#example-2) and [3](#example-3):

* [pandas (1.0.1)](https://pandas.pydata.org/)
* [Fiona (1.8.6)](https://fiona.readthedocs.io/)
* [Shapely (1.6.4)](https://fiona.readthedocs.io/)

The software can be installed under a virtual environment, although it isn't strictly necessary.

```sh
git clone https://github.com/pinno/shadow-mapper.git
cd shadow-mapper
python setup.py install
```


## Example 1

Given an elevation raster and a time (preferably in daylight), the algorithm computes which pixels of the raster are lit by the sun and which are in shade at that time. A picture showing the shadows is also produced.

```python
from datetime import datetime
from shadows.raster import Raster
from shadows.shadowmap import ShadowMap

# Instantiate Raster object
dsm = Raster(filename='example/557140_12_WGS.ASC')

# Time in UTC
t = datetime.now()

# Instantiate Shadow Map object
sm = ShadowMap(dsm=dsm, date_time=t)

# Compute shadows
sm.compute()

# Transform shadows to image and save it as PNG file
sm.to_image().save('example/shadow_map.png')
```

## Example 2

Given an elevation raster and a year long time series with hourly frequency, the algorithm computes which pixels of the raster are lit by the sun and which are in shade for each of the times, then it sums up the shadows and produces a picture every 7 days showing the cumulative shadows. Finally, a video is created by using the single pictures as consecutive frames. Warning: this example takes a few minutes because it computes the shadows for 8736 different times of the year, and not in a smart way.

```python
from datetime import datetime
from numpy import uint8, zeros
from pandas import date_range
from shadows.raster import Raster
from shadows.shadowmap import ShadowMap
from PIL import Image

# Instantiate Raster object
dsm = Raster(filename='example/557140_12_WGS.ASC')

# Define hourly time series for a whole year
hours = date_range(start='2020-01-01', end='2021-01-01', freq='1h', closed='left')

# Iterate over groups of 7 days
for week in range(1, 53):

    # Initialize cumulative sum of shadows
    shadow_sum = zeros(dsm.elevation_map.shape)

    # Compute indexes to select the timestamps for the current week
    week_start_idx = (week - 1) * 24 * 7
    week_end_idx = week * 24 * 7 - 1

    print(f'Computing shadows for week {week} [hours {week_start_idx} to {week_end_idx}]...')

    # Iterate over the hours of the current week
    for t_idx, t in enumerate(hours[week_start_idx:week_end_idx+1]):

        # Compute shadow and add it to the sum matrix
        shadow_sum = shadow_sum + ShadowMap(dsm=dsm, date_time=t).compute()

    # Scale image values between 0 and 255
    image = (255.0 / shadow_sum.max() * (shadow_sum - shadow_sum.min())).astype(uint8)

    # Save image to PNG file
    Image.fromarray(image).save(f'example/shadows_sum_week_{week:02}.png')
```

```sh
# Create a video animation of the above weekly cumulative sums of shadows:
ffmpeg -r 2 -i example/shadows_sum_week_%2d.png -vcodec libx264 -y -an example/shadow_map_video.mp4 -vf "pad=ceil(iw/2)*2:ceil(ih/2)*2"
```

## Example 3

The following procedure illustrates how the ShadowIndex class has been used to compute the shadows exclusively for the raster pixels within the building boundaries, i.e. a small fraction of the total pixels, as also shown in the paper [A procedure for complete census estimation of rooftop photovoltaic potential in urban areas](https://www.mdpi.com/journal/smartcities) published for the journal [Smart Cities](https://www.mdpi.com/journal/smartcities).

```python
import fiona
import numpy as np
import rasterio
from datetime import datetime
from math import ceil, floor, cos, sin, radians
from pvlib.solarposition import get_solarposition
from shapely.geometry import shape, Polygon
from shadows.raster import Raster
from shadows.shadowmap import ShadowIndex
from PIL import Image

# ---------------------------------------------------------------------------- #
# Selection of pixels corresponding to raster cells entirely within buildings
# ---------------------------------------------------------------------------- #

# Read building shapefile
building_shapefile = 'example/buildings.shp'
buildings = fiona.open(building_shapefile)

# Read elevation raster metadata
elevation_raster = 'example/557140_12_WGS.ASC'
with rasterio.open(elevation_raster) as raster:
    y_top = raster.profile['transform'][5]
    x_left = raster.profile['transform'][2]
    n_rows = raster.profile['height']
    n_cols = raster.profile['width']
    cell_size = raster.res[0]
    half_cell_size = 0.5 * cell_size

# Build raster envelope
raster_lt = (x_left, y_top)
raster_rt = (x_left + n_cols, y_top)
raster_rb = (x_left + n_cols, y_top - n_rows)
raster_lb = (x_left, y_top - n_rows)
raster_bb = Polygon([raster_lt, raster_rt, raster_rb, raster_lb, raster_lt])

# Initialize arrays to contain the coordinates of the cells within buildings
xs = []
ys = []

# Iterate over buildings
for building in buildings:
    # Shape of the current building
    building_geometry = shape(building['geometry'])
    # If the building is entirely within the boundaries of the elevation raster
    if building_geometry.within(raster_bb):
        # Detect the max and the min coordinates for the cells within the building
        x_min = floor(building_geometry.bounds[0]) + half_cell_size
        y_min = floor(building_geometry.bounds[1]) + half_cell_size
        x_max = ceil(building_geometry.bounds[2]) - half_cell_size
        y_max = ceil(building_geometry.bounds[3]) - half_cell_size
        # Number of possible cells in both X and Y directions
        n_xs = int(np.round(x_max - x_min))
        n_ys = int(np.round(y_max - y_min))
        # Iterate over X axis
        for x_idx in range(n_xs):
            # Get x_min and x_max coordinates of the current cell
            qx_min = x_min + x_idx * cell_size
            qx_max = qx_min + cell_size
            # Iterate over Y axis
            for y_idx in range(n_ys):
                # Get y_min and y_max coordinates of the current cell
                qy_min = y_min + y_idx * cell_size
                qy_max = qy_min + cell_size
                # Define the current cell as a polygon
                cell_polygon = Polygon([[qx_min, qy_min], [qx_max, qy_min],
                                        [qx_max, qy_max], [qx_min, qy_max],
                                        [qx_min, qy_min]])
                # Append if the cell is entirely within the shape of the building
                if cell_polygon.within(building_geometry):
                    xs.append(qx_min + half_cell_size)
                    ys.append(qy_min + half_cell_size)

# Transform cell coordinates into pixel coordinates
row_idxs = np.round(y_top - 0.5 - np.array(ys)).astype(np.uint16)
col_idxs = np.round(np.array(xs) - 0.5 - x_left).astype(np.uint16)
row_idxs = np.reshape(row_idxs, (row_idxs.size, 1))
col_idxs = np.reshape(col_idxs, (col_idxs.size, 1))

# ---------------------------------------------------------------------------- #
# Shadow mapping for the selected pixels
# ---------------------------------------------------------------------------- #

# Instantiate Raster object
dsm = Raster(filename=elevation_raster)

# Time in UTC
t = datetime.now()

# Compute sun position for the given time and location
sp = get_solarposition(t, dsm.lat, dsm.lng)
altitude = radians(sp['elevation'])
azimuth = radians(sp['azimuth'])
sun_x = dsm.precision(sin(azimuth) * cos(altitude))
sun_y = dsm.precision(-cos(azimuth) * cos(altitude))
sun_z = dsm.precision(sin(altitude) * dsm.resolution)

# Instantiate Shadow Index object
sm = ShadowIndex(dsm=dsm, row_idxs=row_idxs, col_idxs=col_idxs,
                 sun_x=sun_x, sun_y=sun_y, sun_z=sun_z)

# Compute shadows for a selection of pixels
sm.compute()

# Transform shadows to image and save it as PNG file
sm.to_image().save('example/shadow_index.png')
```

## Data

The folder `example` of the repository contains the files used as inputs for the above examples:

* `557140_12_WGS.ASC` is a [1141 x 1466 raster with cell resolution of 1 m2](http://webgis.regione.sardegna.it/scaricocartografiaETL/modelliDigitali/Fascia_costiera_2008_DSM1m/557140_12.ZIP), representing the digital surface model for a portion of the city of Cagliari. The raster files are provided by Regione Autonoma Sardegna through the web service [SardegnaMappe](http://www.sardegnageoportale.it/webgis2/sardegnamappe/?map=download_raster). Digital models for other areas in Sardinia are available for download and are shown in the map by selecting *"DTM-DSM passo 1 m fascia costiera 2008"* on the left menu.
* `557140_12_WGS.prj` contains the projection details for the above raster.
* `buildings.shp`, `buildings.dbf`, `buildings.shx` and `buildings.prj` are a collection of files representing the shapefile describing the buildings within the city of Cagliari. The content of this shapefile is a subset of a larger shapefile from *"Strato 02"* of the [DataBase GeoTopografico della Sardegna](http://www.sardegnageoportale.it/index.php?xsl=2425&s=391170&v=2&c=14414&t=1&tb=14401).