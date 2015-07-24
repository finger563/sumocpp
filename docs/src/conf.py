project = 'Editor'
version = '0.1'
release = '0.1.0'
copyright = '2015, finger563'

import os,inspect,sys
sumocpp = os.path.realpath(os.path.abspath
                           (os.path.join
                            (os.path.split
                             (inspect.getfile
                              (inspect.currentframe()
                           )
                          )[0], "../../src/")
                        ))
if sumocpp not in sys.path:
    sys.path.insert(0, sumocpp)

master_doc = 'index'
source_suffix = '.rst'
exclude_patterns = ['**/.#*']
extensions = ['sphinx.ext.pngmath', 'sphinx.ext.autodoc', 'sphinxcontrib.spelling', 'sphinx.ext.todo']
templates_path = ['_templates']
autoclass_content = "both"
autodoc_member_order = "bysource"

pygments_style = 'sphinx'
import sphinx_rtd_theme
html_theme = 'sphinx_rtd_theme'
html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
htmlhelp_basename = 'cbsat-doc'
html_static_path = ['_static']
html_context = { 'css_files': ['./_static/custom.css'] }

todo_include_todos = True

spelling_word_list_filename = 'dictionary.txt'
