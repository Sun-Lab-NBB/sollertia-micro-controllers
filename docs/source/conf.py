# Configuration file for the Sphinx documentation builder.

# -- Project information -----------------------------------------------------
project = 'sl-micro-controllers'
# noinspection PyShadowingBuiltins
copyright = '2025, Sun (NeuroAI) lab'
authors = ['Ivan Kondratyev']
release = '3.0.0'

# -- General configuration ---------------------------------------------------
extensions = [
    'breathe',  # To read doxygen-generated xml files (to parse C++ documentation).
]

templates_path = ['_templates']
exclude_patterns = []

# Breathe configuration
breathe_projects = {"sl-micro-controllers": "./doxygen/xml"}
breathe_default_project = "sl-micro-controllers"
breathe_doxygen_config_options = {
    'ENABLE_PREPROCESSING': 'YES',
    'MACRO_EXPANSION': 'YES',
    'EXPAND_ONLY_PREDEF': 'NO',
    'PREDEFINED': 'PACKED_STRUCT=',
}

# -- Options for HTML output -------------------------------------------------
html_theme = 'furo'
