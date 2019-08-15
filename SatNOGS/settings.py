from decouple import config

# Basic settings
DB_BASE_URL = config('DB_BASE_URL', default='https://db.satnogs.org')
NETWORK_BASE_URL = config('NETWORK_BASE_URL', default='https://network.satnogs.org')
CACHE_AGE = config('CACHE_AGE', default=24)  # In hours
MAX_NORAD_CAT_ID = config('MAX_NORAD_CAT_ID', default=90000)
MIN_PASS_DURATION = config('MIN_PASS_DURATION', default=2)  # In minutes

# Credentials
NETWORK_USERNAME = config('NETWORK_USERNAME', default='')
NETWORK_PASSWORD = config('NETWORK_PASSWORD', default='')
