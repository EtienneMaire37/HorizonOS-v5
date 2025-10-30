#pragma once

#define lapic_reg_offset(reg, next_reg) ((next_reg) - (reg) - 4)
#define lapic_reg_next_offset()         (0x10 - 4)

struct local_apic_isr_register
{
    uint32_t value;
    uint8_t reserved[lapic_reg_next_offset()];
} __attribute__((packed));

struct local_apic_tmr_register
{
    uint32_t value;
    uint8_t reserved[lapic_reg_next_offset()];
} __attribute__((packed));

struct local_apic_irr_register
{
    uint32_t value;
    uint8_t reserved[lapic_reg_next_offset()];
} __attribute__((packed));

struct local_apic_icr_register
{
    uint32_t value;
    uint8_t reserved[lapic_reg_next_offset()];
} __attribute__((packed));

struct local_apic_registers
{
    uint8_t reserved0[0x20];
    uint32_t id_register;
    uint8_t reserved1[lapic_reg_next_offset()];
    uint32_t version_register;
    uint8_t reserved2[lapic_reg_offset(0x30, 0x80)];
    uint32_t task_priority_register;
    uint8_t reserved3[lapic_reg_next_offset()];
    uint32_t arbitration_priority_register;
    uint8_t reserved4[lapic_reg_next_offset()];
    uint32_t processor_priority_register;
    uint8_t reserved5[lapic_reg_next_offset()];
    uint32_t end_of_interrupt_register;
    uint8_t reserved6[lapic_reg_next_offset()];
    uint32_t remote_read_register;
    uint8_t reserved7[lapic_reg_next_offset()];
    uint32_t logical_destination_register;
    uint8_t reserved8[lapic_reg_next_offset()];
    uint32_t destination_format_register;
    uint8_t reserved9[lapic_reg_next_offset()];
    uint32_t spurious_interrupt_vector__register;
    uint8_t reserved10[lapic_reg_next_offset()];
    struct local_apic_isr_register in_service_registers[8];
    struct local_apic_tmr_register trigger_mode_registers[8];
    struct local_apic_irr_register interrupt_request_registers[8];
    uint32_t error_status_register;
    uint8_t reserved11[lapic_reg_offset(0x280, 0x2f0)];
    uint32_t lvt_corrected_machine_check_interrupt_register;
    uint8_t reserved12[lapic_reg_next_offset()];
    struct local_apic_icr_register interrupt_command_registers[2];
    uint32_t lvt_timer_register;
    uint8_t reserved13[lapic_reg_next_offset()];
    uint32_t lvt_thermal_sensor_register;
    uint8_t reserved14[lapic_reg_next_offset()];
    uint32_t lvt_performance_monitoring_counters_register;
    uint8_t reserved15[lapic_reg_next_offset()];
    uint32_t lvt_lint0_register;
    uint8_t reserved16[lapic_reg_next_offset()];
    uint32_t lvt_lint1_register;
    uint8_t reserved17[lapic_reg_next_offset()];
    uint32_t lvt_error_register;
    uint8_t reserved18[lapic_reg_next_offset()];
    uint32_t initial_count_register;
    uint8_t reserved19[lapic_reg_next_offset()];
    uint32_t current_count_register;
    uint8_t reserved20[lapic_reg_offset(0x390, 0x3e0)];
    uint32_t divide_configuration_register;
} __attribute__((packed));

volatile struct local_apic_registers* lapic;