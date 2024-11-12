
void write_apic(uint32_t reg, uint32_t value);
uint32_t read_apic(uint32_t reg);

void init_apic();
void apic_send_eoi();
