# Copyright 2020 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""env_setup module definition for PyOxidizer."""

import unittest
import setuptools


def test_suite():
    """Test suite for env_setup module."""
    return unittest.TestLoader().discover('./', pattern='*_test.py')


setuptools.setup(
    name='pw_env_setup',
    version='0.0.1',
    author='Pigweed Authors',
    author_email='pigweed-developers@googlegroups.com',
    description='Environment setup for Pigweed',
    packages=setuptools.find_packages(),
    test_suite='setup.test_suite',
    entry_points={
        'console_scripts': ['_pw_env_setup = pw_env_setup.env_setup:main'],
    },
    package_data={
        'pw_env_setup': [
            'cargo_setup/packages.txt',
            'cipd_setup/luci.json',
            'cipd_setup/pigweed.json',
            'virtualenv_setup/requirements.in',
            'virtualenv_setup/requirements.txt',
        ],
    },
)
