# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os

# -- Project information -----------------------------------------------------

project = 'OpenROAD'
copyright = 'The Regents of the University of California, 2021'
author = 'OpenROAD Team'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.ifconfig',
    'sphinx.ext.mathjax',
    'sphinx.ext.napoleon',
    'sphinx.ext.todo',
    'sphinx_external_toc',
    'myst_parser',
]

myst_enable_extensions = [
    'amsmath',
    'colon_fence',
    'deflist',
    'dollarmath',
    'html_admonition',
    'html_image',
    'replacements',
    'smartquotes',
    'substitution',
    'tasklist',
    'html_image',
]

external_toc_path = 'toc.yml'

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
# source_suffix = ['.rst', '.md']
source_suffix = ['.md']

# The master toctree document.
master_doc = 'index'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = [
    '_build',
    'Thumbs.db',
    '.DS_Store',
    '**/LICENSE',
    '**/LICENSE.md',
    'README.md',
    'misc/NewToolDocExample.md',
    'docs/releases/PostAlpha2.1BranchMethodology.md',
    'main/src/odb/src/def/README.md',
    'main/src/odb/src/def/doc/README.md',
    'main/src/odb/src/lef/README.md',
    'main/docs',
]

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = None


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_symbiflow_theme"

html_theme_options = {
    # Repository integration
    # Set the repo url for the link to appear
    'github_url': 'https://github.com/The-OpenROAD-Project/OpenROAD',
    # The name of the repo. If must be set if github_url is set
    'repo_name': 'OpenROAD',
    # Must be one of github, gitlab or bitbucket
    'repo_type': 'github',

    # Set the name to appear in the left sidebar/header. If not provided, uses
    # html_short_title if defined, or html_title
    'nav_title': "OpenROAD",

    # A list of dictionaries where each has three keys:
    #   href: The URL or pagename (str)
    #   title: The title to appear (str)
    #   internal: Flag indicating to use pathto (bool)
    'nav_links': [
        {"title": "Home", "href": "index", "internal": True},
        {"title": "The OpenROAD Project",
            "href": "https://theopenroadproject.org", "internal": False},
    ],

    # Customize css colors.
    # For details see link.
    # https://getmdl.io/customize/index.html
    #
    # Primary colors:
    # red, pink, purple, deep-purple, indigo, blue, light-blue, cyan,
    # teal, green, light-green, lime, yellow, amber, orange, deep-orange,
    # brown, grey, blue-grey, white
    # (Default: deep-purple)
    'color_primary': 'indigo',
    # Values: Same as color_primary.
    # (Default: indigo)
    'color_accent': 'blue',

    # Hide the symbiflow links
    'hide_symbiflow_links': True,

    "html_minify": False,
    "html_prettify": True,
    "css_minify": True,
    "globaltoc_depth": 2,
    "table_classes": ["plain"],
    "master_doc": False,
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
# html_static_path = ['_static']


def swap_prefix(file, old, new):
    with open(file, 'r') as f:
        lines = f.read()
    lines = lines.replace(old, new)
    with open(file, 'wt') as f:
        f.write(lines)


def setup(app):
    import os

    if not os.path.exists('./main'):
        os.symlink('..', './main')
    swap_prefix('../README.md', '(docs/', '(../')
