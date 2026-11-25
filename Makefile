# Genesis Mini firmware: the only entry point to build, test, flash and monitor.
#
# Each project is an [env:<name>] in platformio.ini with its code in src/<name>/.
# PROJECT=<name> picks one; `make projects` lists them.
SHELL := /bin/bash
.DEFAULT_GOAL := help

VENV  := .venv
PIO   := $(VENV)/bin/pio
STAMP := $(VENV)/.setup-done
export PLATFORMIO_CORE_DIR := $(CURDIR)/.pio-core

PLATFORMIO_VERSION   := 6.2.0
AXIOMETA_CLI_VERSION := 2.0.2

ESPTOOL    := .pio-core/penv/bin/esptool
BACKUP_DIR := backups

# Every [env:...] in platformio.ini except the host test environment.
PROJECTS := $(filter-out native,$(patsubst [env:%],%,$(shell grep -oE '^\[env:[a-z0-9_]+\]' platformio.ini)))

.PHONY: help setup projects build test ports upload monitor log backup restore clean verify check-project check-port check-esptool

help: ## List targets
	@grep -E '^[a-z]+:.*## ' $(MAKEFILE_LIST) | awk -F':.*## ' '{printf "  %-9s %s\n", $$1, $$2}'

$(STAMP): Makefile
	python3 -m venv $(VENV)
	$(VENV)/bin/pip install --disable-pip-version-check \
	  platformio==$(PLATFORMIO_VERSION) axiometa-cli==$(AXIOMETA_CLI_VERSION)
	touch $@

setup: $(STAMP) ## Create .venv/ and install the toolchain into .pio-core/
	$(PIO) pkg install $(addprefix -e ,$(PROJECTS)) -e native

projects: ## List the projects that can be built and flashed
	@for p in $(PROJECTS); do echo "  $$p"; done

build: $(STAMP) $(if $(PROJECT),check-project) ## Compile every project, or one: make build PROJECT=hello
	$(PIO) run $(addprefix -e ,$(or $(PROJECT),$(PROJECTS)))

test: $(STAMP) ## Run the native unit tests on this computer
	$(PIO) test -e native

ports: $(STAMP) ## List serial ports, to choose PORT
	$(PIO) device list --serial

upload: $(STAMP) check-project check-port ## Flash a project: make upload PROJECT=hello PORT=/dev/cu.usbmodemXXXX
	$(PIO) run -e $(PROJECT) -t upload --upload-port $(PORT)

monitor: $(STAMP) check-port ## Serial monitor: make monitor PORT=/dev/cu.usbmodemXXXX
	$(PIO) device monitor -p $(PORT) -b 115200

log: $(STAMP) check-port ## Record serial output for a while: make log PORT=/dev/cu.usbmodemXXXX SECONDS=10
	$(VENV)/bin/python -I scripts/serial_log.py $(PORT) $(or $(SECONDS),10)

backup: $(STAMP) check-port check-esptool ## Save the board's whole 4 MB flash: make backup PORT=/dev/cu.usbmodemXXXX
	@mkdir -p $(BACKUP_DIR)
	@f=$(BACKUP_DIR)/genesis_mini-$$(date +%Y%m%d-%H%M%S).bin; \
	$(ESPTOOL) --chip esp32s3 --port $(PORT) --no-stub read-flash 0 ALL $$f && \
	$(ESPTOOL) --chip esp32s3 --port $(PORT) --no-stub verify-flash 0 $$f && \
	shasum -a 256 $$f > $$f.sha256 && echo "Backup saved and verified against the board: $$f"

restore: $(STAMP) check-port check-esptool ## Write a backup over the WHOLE flash (ask first): make restore PORT=... BACKUP=backups/<file>.bin
	@test -f "$(BACKUP)" || { echo "BACKUP=<file> is required, see $(BACKUP_DIR)/"; exit 1; }
	shasum -a 256 -c $(BACKUP).sha256
	$(ESPTOOL) --chip esp32s3 --port $(PORT) --baud 921600 write-flash 0 $(BACKUP)

clean: ## Remove build output (keeps the toolchain)
	rm -rf .pio/build

verify: build test ## The gate: every project builds warning-free and the tests pass

check-project:
	@test -n "$(PROJECT)" || { echo "PROJECT is required, one of: $(PROJECTS)"; exit 1; }
	@case " $(PROJECTS) " in *" $(PROJECT) "*) ;; *) echo "Unknown PROJECT '$(PROJECT)', expected one of: $(PROJECTS)"; exit 1;; esac

check-port:
	@test -n "$(PORT)" || { echo "PORT is required, e.g. PORT=/dev/cu.usbmodem1101 (see 'make ports')"; exit 1; }
	@test -e "$(PORT)" || { echo "$(PORT) does not exist (see 'make ports')"; exit 1; }

check-esptool:
	@test -x $(ESPTOOL) || { echo "Run 'make setup' first"; exit 1; }
