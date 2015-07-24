SRC_DIR = src
API_DIR = api
BUILD_DIR = build
IMAGE_DIR = src/images
PYTHON_API_DIR = src/python-api
GH_PAGES_DIR = output

SPHINX_BUILD = sphinx-build
SPHINX_AUTOBUILD = sphinx-autobuild
SPHINX_OPTS = -d $(BUILD_DIR)/doctrees $(SRC_DIR)

# Main targets -----------------------------------------------------------------

.PHONY: all build clean

all: build

build: html

clean:
	rm -rf $(BUILD_DIR) $(PYTHON_API_DIR) $(GH_PAGES_DIR)

# Build specific types of doc---------------------------------------------------

.PHONY: html pdf gh-pages

spelling: api
	$(SPHINX_BUILD) -b spelling $(SPHINX_OPTS) $(BUILD_DIR)/spelling

html: api
	$(SPHINX_BUILD) -b html $(SPHINX_OPTS) $(BUILD_DIR)/html

pdf: api
	$(SPHINX_BUILD) -b latex $(SPHINX_OPTS) $(BUILD_DIR)/pdf
	make -C $(BUILD_DIR)/pdf

gh-pages: api
	rm -rf $(GH_PAGES_DIR)
	$(SPHINX_BUILD) -b html $(SPHINX_OPTS) $(GH_PAGES_DIR)

# API docs ---------------------------------------------------------------------

.PHONY: api python-api

api: check-api python-api

check-api:
	./check-api.py $(API_DIR) services.yaml

python-api:
	./build-api.py $(API_DIR) $(PYTHON_API_DIR)

# Sphinx server on localhost ---------------------------------------------------

.PHONY: serve wait-api-changed sphinx-autobuild

serve: html
	python serve.py

wait-api-changed:
	inotifywait -r -e modify,move,create,delete $(API_DIR)

sphinx-autobuild:
	$(SPHINX_AUTOBUILD) -b html $(SPHINX_OPTS) $(BUILD_DIR)/html
