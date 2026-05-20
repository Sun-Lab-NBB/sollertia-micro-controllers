# Configuration file for the Sphinx documentation builder.

# -- Project information -----------------------------------------------------
project = 'sollertia-micro-controllers'
# noinspection PyShadowingBuiltins
copyright = '2026, Sun (NeuroAI) lab'
authors = ['Ivan Kondratyev']
release = '4.0.0'

# -- General configuration ---------------------------------------------------
extensions = [
    'breathe',             # To read doxygen-generated xml files (to parse C++ documentation).
]

# Breathe configuration
breathe_projects = {"sollertia-micro-controllers": "./doxygen/xml"}
breathe_default_project = "sollertia-micro-controllers"
breathe_doxygen_config_options = {
    'ENABLE_PREPROCESSING': 'YES',
    'MACRO_EXPANSION': 'YES',
    'EXPAND_ONLY_PREDEF': 'NO',
    'PREDEFINED': 'PACKED_STRUCT='
}

# -- Options for HTML output -------------------------------------------------
html_theme = 'furo'
