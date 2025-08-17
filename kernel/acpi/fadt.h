#pragma once

#include <kernel/acpi/defs.h>

enum {
    WBINVD,
    WBINVD_FLUSH,
    PROC_C1,
    P_LVL2_UP,
    PWR_BUTTON,
    SLP_BUTTON,
    FIX_RTC,
    RTC_S4,
    TMR_VAL_EXT,
    DCK_CAP,
    RESET_REG_SUP,
    SEALED_CASE,
    HEADLESS,
    CPU_SW_SLP,
    PCI_EXP_WAK,
    USE_PLATFORM_CLOCK,
    S4_RTC_STS_VALID,
    REMOTE_POWER_ON_CAPABLE,
    FORCE_APIC_CLUSTER_MODEL,
    FORCE_APIC_PHYSICAL_DESTINATION_MODE
};

typedef struct {
    header_t hdr;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
    uint32_t gpe0_blk;
    uint32_t gpe1_blk;
    uint8_t pm1_evt_len;
    uint8_t pm1_cnt_len;
    uint8_t pm2_cnt_len;
    uint8_t pm_tmr_len;
    uint8_t gpe0_blk_len;
    uint8_t gpe1_blk_len;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t mon_alarm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
    gas_t reset_reg;
    uint8_t reset_value;
    uint8_t reserved3[3];
    uint64_t x_firmware_control;
    uint64_t x_dsdt;
    gas_t x_pm1a_evt_blk;
    gas_t x_pm1b_evt_blk;
    gas_t x_pm1a_cnt_blk;
    gas_t x_pm1b_cnt_blk;
    gas_t x_pm2_cnt_blk;
    gas_t x_pm_tmr_blk;
    gas_t x_gpe0_blk;
    gas_t x_gpe1_blk;
    gas_t sleep_control_reg;
    gas_t sleep_status_reg;
} __attribute__((packed)) fadt_t;

fadt_t *acpi_fadt_table();
int acpi_fadt_flag(uint32_t);
void acpi_fadt_parse(uint64_t);
void acpi_ssdt_parse(uint64_t);
