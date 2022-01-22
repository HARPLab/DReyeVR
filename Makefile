default: help

help:
	@less Scripts/DReyeVR.mk.help

carla: install

install:
	@Scripts/install.sh --carla ${CARLA}

# clean: # TODO: clean? (git reset --hard? git clean -fd?)
# 	@Scripts/clean.sh

# check: # TODO: make DReyeVR unit tests!
# 	@Scripts/check.sh

sr: scenario-runner

scenario-runner:
	@Scripts/install.sh --scenario-runner ${SR}

patch-sranipal:
	@Scripts/patch_sranipal.sh ${CARLA}

all: carla scenario-runner patch-sranipal patch-logitech