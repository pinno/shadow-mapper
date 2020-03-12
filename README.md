
# Shadow Mapper

This software is a lightweight version of [shadow-mapper](https://github.com/perliedman/shadow-mapper): given an elevation raster, it computes which parts of the geographic area are lit by the sun and which are in shade at a certain time.


## Installation

It works with [Python 3.7](https://www.python.org/). The following libraries are needed:

* [Pillow (7.0.0)](https://python-pillow.org/)
* [astropy  (4.0)](https://www.astropy.org/)
* [numpy (1.18.1)](https://numpy.org/)
* [rasterio (1.1.1)](https://rasterio.readthedocs.io/en/latest/)

The software can be installed under a virtual environment, although it isn't strictly necessary.

```sh
cd shadow-mapper
python setup.py install
```

## Example

```python
from datetime import datetime
import numpy as np
import os
from shadows.shadowmap import ShadowMap
from shadows.raster import Raster

# Path to the elevation raster file
raster_file = 'DEM_RASTER.ASC'

# Instantiate Raster object
dem = Raster(filename=raster_file)

# Time in UTC
t = datetime.now()

# Compute shading
sm = ShadowMap(dem=dem, date_time=t, view_height=0.0)

# Save image as PNG file
sm.to_image().save('SHADOW_MAP.PNG')

```