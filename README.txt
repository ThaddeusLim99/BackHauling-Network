Offloading/backhauling networks have gained considerable attention for their ability to transmit sensor data from environments lacking conventional network access, like cellular towers. In these setups, devices such as smartphones, laptops, and tablets receive transmissions from sensors and transmit them to a cloud server whenever they connect to a traditional network. Prominent examples include Apple AirTags.

Steps to compile and execute programs:

1. ensure Source-Code directory is in contiki-ng/examples/
2. cd into contiki-ng/examples/Source-Code
3. make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 task2_group_13_A
4. make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 task2_group_13_B
5. load the compiled task2_A into one sensor tag using uniflash
6. load the compiled task2_B into another sensor tag using uniflash
