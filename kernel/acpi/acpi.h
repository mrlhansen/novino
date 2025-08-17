#pragma once

#include <kernel/types.h>

void acpi_init(uint64_t);
void acpi_enable();

int acpi_8042_support();
uint64_t acpi_hpet_address();
uint64_t acpi_lapic_address();
uint64_t acpi_mcfg_entries(int*);

void acpi_report_ioapic();
void acpi_report_iso();
void acpi_report_core();
