/* Case 13, fixed: one header declares every vector-table entry. */
#ifndef HANDLERS_H
#define HANDLERS_H

void default_handler(void);
void uart1_irq_handler(void);
void timer0_irq_handler(void);

#endif
