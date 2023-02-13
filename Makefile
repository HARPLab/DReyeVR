default: help

help:
	@less Scripts/DReyeVR.mk.help

DReyeVR: install

install:
	python Scripts/install.py --carla ${CARLA} --scenario-runner ${SR}

clean:
	python Scripts/clean.py --carla ${CARLA} --scenario-runner ${SR} --verbose

test:
	python Scripts/tests.py

check: # TODO: make better DReyeVR unit tests!
	python Scripts/check_install.py --carla ${CARLA} --scenario-runner ${SR} --verbose

rev: r-install

r-install:
	python Scripts/r-install.py --carla ${CARLA} --scenario-runner ${SR}

all: install check