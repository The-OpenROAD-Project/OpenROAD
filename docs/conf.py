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
    'sphinx_copybutton',
    'myst_parser',
    'sphinxcontrib.mermaid'
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

# Mermaid related args
mermaid_output_format = 'svg'
mermaid_params = ['-p', 'puppeteer-config.json',
                  '--theme', 'forest',
                  '--width', '200',
                  '--backgroundColor', 'transparent']
mermaid_init_js = "mermaid.initialize({startOnLoad:true, flowchart:{useMaxWidth:false}})"
mermaid_verbose = True

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_book_theme"

html_theme_options = {
    "repository_url": "https://github.com/The-OpenROAD-Project/OpenROAD",
    "repository_branch": "master",
    "show_navbar_depth": 2,
    "use_issues_button": True,
    # "use_repository_button": True,
    "use_download_button": True,

    # list for more fine-grained ordering of icons
    "icon_links": [
        {
            "name": "The OpenROAD Project",
            "url": "https://theopenroadproject.org/",
            "icon": "fa-solid fa-globe",
        },
        {
            "name": "Twitter",
            "url": "https://twitter.com/OpenROAD_EDA",
            "icon": "fa-brands fa-twitter",
        },
        {
            "name": "Email",
            "url": "mailto:openroad@eng.ucsd.edu",
            "icon": "fa-solid fa-envelope",
        },
        {
            "name": "GitHub",
            "url": "https://github.com/The-OpenROAD-Project/OpenROAD",
            "icon": "fa-brands fa-github",
        },
        {
            "name": "Stars",
            "url": "https://github.com/The-OpenROAD-Project/OpenROAD/stargazers",
            "icon": "https://img.shields.io/github/stars/The-OpenROAD-Project/OpenROAD",
            "type": "url",
        },
   ],
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
    # these prefix swaps will be reverted and is needed for sphinx compilation.
    swap_prefix('../README.md', '(docs/', '(../')
    swap_prefix('../README.md', '```mermaid', '```{mermaid}\n:align: center\n')

    # for populating OR Messages page.
    command = "python getMessages.py"
    _ = os.popen(command).read()
