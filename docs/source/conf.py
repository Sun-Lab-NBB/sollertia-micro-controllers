# Configuration file for the Sphinx documentation builder.

# -- Project information -----------------------------------------------------
project = 'sollertia-micro-controllers'
# noinspection PyShadowingBuiltins
copyright = '2026, Sun (NeuroAI) lab'
author = 'Ivan Kondratyev'
release = '4.0.0'

# -- General configuration ---------------------------------------------------
extensions = [
    'breathe',             # To read doxygen-generated xml files (to parse C++ documentation).
]

# Breathe configuration
breathe_projects = {"sollertia-micro-controllers": "./doxygen/xml"}
breathe_default_project = "sollertia-micro-controllers"

# -- Options for HTML output -------------------------------------------------
html_theme = 'furo'
