
void write_apic(uint32_t reg, uint32_t value);
uint32_t read_apic(uint32_t reg);

void init_apic();
void setup_apic_timer(uint32_t interval);
void apic_send_eoi();
uintptr_t get_apic_virtual_base();
