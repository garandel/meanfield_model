# -*- coding: utf-8 -*-
#
# Copyright (c) 2017-2019 The University of Manchester
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# data_allocation documentation build configuration file, created by
# sphinx-quickstart on Tue Jun 17 08:56:46 2014.
#
# This file is execfile()d with the current directory set to its
# containing dir.
#
# Note that not all possible configuration values are present in this
# autogenerated file.
#
# All configuration values have a default; values that are commented out
# serve to show the default.

import mock
import os
import sys

autodoc_mock_imports = ['_tkinter']

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
# sys.path.insert(0, os.path.abspath('.'))

# -- General configuration ------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'sphinx.ext.autosummary',
    'sphinx.ext.intersphinx',
    'sphinx.ext.mathjax'
]

intersphinx_mapping = {
    'python': ('https://docs.python.org/3.6', None),
    'numpy': ("https://numpy.org/doc/1.19/", None),
    'pynn': ("http://neuralensemble.org/docs/PyNN/", None),
    'spinn_utilities': ('https://spinnutils.readthedocs.io/en/latest/', None),
    'spinn_machine': ('https://spinnmachine.readthedocs.io/en/latest/', None),
    'spinnman': ('https://spinnman.readthedocs.io/en/latest/', None),
    'pacman': ('https://pacman.readthedocs.io/en/latest/', None),
    'data_specification': (
        'https://dataspecification.readthedocs.io/en/latest/', None),
    'spinn_front_end_common': (
        'https://spinnfrontendcommon.readthedocs.io/en/latest/', None)
}

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# The suffix of source filenames.
source_suffix = '.rst'

# The encoding of source files.
# source_encoding = 'utf-8-sig'

# The master toctree document.
master_doc = 'index'

# General information about the project.
project = u'sPyNNaker'
copyright = u'2014-2017'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
version = ''
# The full version, including alpha/beta/rc tags.
release = ''

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
# language = None

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
# today = ''
# Else, today_fmt is used as the format for a strftime call.
# today_fmt = '%B %d, %Y'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ['_build']

# The reST default role (used for this markup: `text`) to use for all
# documents.
# default_role = None

# If true, '()' will be appended to :func: etc. cross-reference text.
# add_function_parentheses = True

# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
# add_module_names = True

# If true, sectionauthor and moduleauthor directives will be shown in the
# output. They are ignored by default.
# show_authors = False

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# A list of ignored prefixes for module index sorting.
# modindex_common_prefix = []

# If true, keep warnings as "system message" paragraphs in the built documents.
# keep_warnings = False


# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
html_theme = 'sphinxdoc'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
# html_theme_options = {}

# Add any paths that contain custom themes here, relative to this directory.
# html_theme_path = []

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
# html_title = None

# A shorter title for the navigation bar.  Default is the same as html_title.
# html_short_title = None

# The name of an image file (relative to this directory) to place at the top
# of the sidebar.
# html_logo = None

# The name of an image file (within the static path) to use as favicon of the
# docs.  This file should be a Windows icon file (.ico) being 16x16 or 32x32
# pixels large.
# html_favicon = None

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['_static']

# Add any extra paths that contain custom files (such as robots.txt or
# .htaccess) here, relative to this directory. These files are copied
# directly to the root of the documentation.
# html_extra_path = []

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
# html_last_updated_fmt = '%b %d, %Y'

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
# html_use_smartypants = True

# Custom sidebar templates, maps document names to template names.
# html_sidebars = {}

# Additional templates that should be rendered to pages, maps page names to
# template names.
# html_additional_pages = {}

# If false, no module index is generated.
# html_domain_indices = True

# If false, no index is generated.
# html_use_index = True

# If true, the index is split into individual pages for each letter.
# html_split_index = False

# If true, links to the reST sources are added to the pages.
# html_show_sourcelink = True

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
# html_show_sphinx = True

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
# html_show_copyright = True

# If true, an OpenSearch description file will be output, and all pages will
# contain a <link> tag referring to it.  The value of this option must be the
# base URL from which the finished HTML is served.
# html_use_opensearch = ''

# This is the file name suffix for HTML files (e.g. ".xhtml").
# html_file_suffix = None

# Output file base name for HTML help builder.
htmlhelp_basename = 'sPyNNakerdoc'

mathjax_path = 'https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.1/MathJax.js?config=TeX-MML-AM_CHTML'

# -- Options for LaTeX output ---------------------------------------------

latex_elements = {
    # The paper size ('letterpaper' or 'a4paper').
    # 'papersize': 'letterpaper',

    # The font size ('10pt', '11pt' or '12pt').
    # 'pointsize': '10pt',

    # Additional stuff for the LaTeX preamble.
    # 'preamble': '',
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
  ('index', 'sPyNNaker.tex',
   u'sPyNNaker Documentation', u'', 'manual'),
]

# The name of an image file (relative to this directory) to place at the top of
# the title page.
# latex_logo = None

# For "manual" documents, if this is true, then toplevel headings are parts,
# not chapters.
# latex_use_parts = False

# If true, show page references after internal links.
# latex_show_pagerefs = False

# If true, show URL addresses after external links.
# latex_show_urls = False

# Documents to append as an appendix to all manuals.
# latex_appendices = []

# If false, no module index is generated.
# latex_domain_indices = True


# -- Options for manual page output ---------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    ('index', 'sPyNNaker',
     u'sPyNNaker Documentation',
     [u''], 1)
]

# If true, show URL addresses after external links.
# man_show_urls = False


# -- Options for Texinfo output -------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
  ('index', 'sPyNNaker',
   u'sPyNNaker Documentation', u'', 'sPyNNaker', '',
   'Miscellaneous'),
]

# Documents to append as an appendix to all manuals.
# texinfo_appendices = []y

# If false, no module index is generated.
# texinfo_domain_indices = True

# How to display URL addresses: 'footnote', 'no', or 'inline'.
# texinfo_show_urls = 'footnote'

# If true, do not generate a @detailmenu in the "Top" node's menu.
# texinfo_no_detailmenu = False


# -- Options for Epub output ----------------------------------------------

# Bibliographic Dublin Core info.
epub_title = u'sPyNNaker'
epub_author = u''
epub_publisher = u''
epub_copyright = u'2014-2017'

# The basename for the epub file. It defaults to the project name.
# epub_basename = u'data_allocation'

# The HTML theme for the epub output.
# Since the default themes are not optimized
# for small screen space, using the same theme for HTML and epub output is
# usually not wise. This defaults to 'epub', a theme designed to save visual
# space.
# epub_theme = 'epub'

# The language of the text. It defaults to the language option
# or en if the language is not set.
# epub_language = ''

# The scheme of the identifier. Typical schemes are ISBN or URL.
# epub_scheme = ''

# The unique identifier of the text. This can be a ISBN number
# or the project homepage.
# epub_identifier = ''

# A unique identification for the text.
# epub_uid = ''

# A tuple containing the cover image and cover page html template filenames.
# epub_cover = ()

# A sequence of (type, uri, title) tuples for the guide element of content.opf.
# epub_guide = ()

# HTML files that should be inserted before the pages created by sphinx.
# The format is a list of tuples containing the path and title.
# epub_pre_files = []

# HTML files shat should be inserted after the pages created by sphinx.
# The format is a list of tuples containing the path and title.
# epub_post_files = []

# A list of files that should not be packed into the epub file.
epub_exclude_files = ['search.html']

# The depth of the table of contents in toc.ncx.
# epub_tocdepth = 3

# Allow duplicate toc entries.
# epub_tocdup = True

# Choose between 'default' and 'includehidden'.
# epub_tocscope = 'default'

# Fix unsupported image types using the PIL.
# epub_fix_images = False

# Scale large images.
# epub_max_image_width = 0

# How to display URL addresses: 'footnote', 'no', or 'inline'.
# epub_show_urls = 'inline'

# If false, no index is generated.
# epub_use_index = True

autoclass_content = 'both'

MOCK_MODULES = ['scipy', 'scipy.stats']
for mod_name in MOCK_MODULES:
    sys.modules[mod_name] = mock.Mock()

sys.path.append(os.path.abspath('../..'))

# Do the rst generation
for f in os.listdir("."):
    if (os.path.isfile(f) and f.endswith(
            ".rst") and f != "index.rst" and f != "modules.rst"):
        os.remove(f)

# We want to document __call__ when encountered
autodoc_default_options = {
    "members": True,
    "special-members": "__call__"
}


def filtered_files(base, excludes=None, exclude_dir=None):
    if not excludes:
        excludes = []
    excludes = set(base + "/" + e for e in excludes)
    for root, _dirs, files in os.walk(base):
        for filename in files:
            full = root + "/" + filename
            if exclude_dir and exclude_dir in root:
                yield full
            elif filename.endswith(".py") and not filename.startswith("_"):
                if full not in excludes:
                    yield full


# UGH!
output_dir = os.path.abspath(".")
os.chdir("../..")

# We only document __init__.py files... except for these special cases.
# Use the unix full pathname from the root of the checked out repo
explicit_wanted_files = [
    "spynnaker/gsyn_tools.py",
    "spynnaker/spike_checker.py",
    "spynnaker/plot_utils.py",
    "spynnaker/pyNN/abstract_spinnaker_common.py",
    "spynnaker/pyNN/exceptions.py",
    "spynnaker/pyNN/spynnaker_simulator_interface.py",
    "spynnaker/pyNN/spynnaker_external_device_plugin_manager.py",
    "spynnaker/pyNN/models/pynn_population_common.py",
    "spynnaker/pyNN/models/pynn_projection_common.py",
    "spynnaker/pyNN/models/defaults.py",
    "spynnaker/pyNN/models/recording_common.py",
    "spynnaker/pyNN/models/neuron/key_space_tracker.py",
    "spynnaker/pyNN/models/neuron/synaptic_matrices.py",
    "spynnaker/pyNN/models/neuron/master_pop_table.py",
    "spynnaker/pyNN/models/neuron/synaptic_matrix.py",
    "spynnaker/pyNN/models/neuron/synapse_io.py",
    "spynnaker/pyNN/models/neuron/synaptic_matrix_app.py",
    "spynnaker/pyNN/models/neuron/plasticity/stdp/common.py",
    "spynnaker/pyNN/models/spike_source/spike_source_array_vertex.py",
    "spynnaker/pyNN/models/spike_source/spike_source_poisson_vertex.py",
    "spynnaker/pyNN/models/spike_source/spike_source_poisson_machine_vertex.py",
    "spynnaker/pyNN/models/common/recording_utils.py",
    "spynnaker/pyNN/utilities/bit_field_utilities.py",
    "spynnaker/pyNN/utilities/spynnaker_failed_state.py",
    "spynnaker/pyNN/utilities/constants.py",
    "spynnaker/pyNN/utilities/extracted_data.py",
    "spynnaker/pyNN/utilities/fake_HBP_Portal_machine_provider.py",
    "spynnaker/pyNN/utilities/running_stats.py",
    "spynnaker/pyNN/utilities/utility_calls.py",
    "spynnaker/pyNN/utilities/struct.py",
    "spynnaker8/spynnaker8_simulator_interface.py",
    "spynnaker8/spynnaker_plotting.py",
    "spynnaker8/utilities/exceptions.py",
    "spynnaker8/utilities/neo_convertor.py",
    "spynnaker8/utilities/neo_compare.py"]
options = ['-o', output_dir, "."]
options.extend(filtered_files(".", explicit_wanted_files, "tests"))
try:
    # Old style API; Python 2.7
    from sphinx import apidoc
    options = [None] + options
except ImportError:
    # New style API; Python 3.6 onwards
    from sphinx.ext import apidoc
apidoc.main(options)
