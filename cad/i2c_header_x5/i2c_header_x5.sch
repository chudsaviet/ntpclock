EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector:Conn_01x05_Male 5V1
U 1 1 6139249A
P 4200 3750
F 0 "5V1" H 4308 4131 50  0000 C CNN
F 1 "Conn_01x05_Male" H 4308 4040 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" H 4200 3750 50  0001 C CNN
F 3 "~" H 4200 3750 50  0001 C CNN
	1    4200 3750
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x05_Male 3V3
U 1 1 61397902
P 4900 3750
F 0 "3V3" H 5008 4131 50  0000 C CNN
F 1 "Conn_01x05_Male" H 5008 4040 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" H 4900 3750 50  0001 C CNN
F 3 "~" H 4900 3750 50  0001 C CNN
	1    4900 3750
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x05_Male GND1
U 1 1 61397CC9
P 5600 3750
F 0 "GND1" H 5708 4131 50  0000 C CNN
F 1 "Conn_01x05_Male" H 5708 4040 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" H 5600 3750 50  0001 C CNN
F 3 "~" H 5600 3750 50  0001 C CNN
	1    5600 3750
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x05_Male SDA1
U 1 1 61397DA7
P 6300 3750
F 0 "SDA1" H 6408 4131 50  0000 C CNN
F 1 "Conn_01x05_Male" H 6408 4040 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" H 6300 3750 50  0001 C CNN
F 3 "~" H 6300 3750 50  0001 C CNN
	1    6300 3750
	1    0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x05_Male SCL1
U 1 1 613984E3
P 7000 3750
F 0 "SCL1" H 7108 4131 50  0000 C CNN
F 1 "Conn_01x05_Male" H 7108 4040 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical" H 7000 3750 50  0001 C CNN
F 3 "~" H 7000 3750 50  0001 C CNN
	1    7000 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	4400 3550 4400 3650
Wire Wire Line
	4400 3650 4400 3750
Connection ~ 4400 3650
Wire Wire Line
	4400 3750 4400 3850
Connection ~ 4400 3750
Wire Wire Line
	4400 3850 4400 3950
Connection ~ 4400 3850
Wire Wire Line
	5100 3550 5100 3650
Wire Wire Line
	5100 3650 5100 3750
Connection ~ 5100 3650
Wire Wire Line
	5100 3750 5100 3850
Connection ~ 5100 3750
Wire Wire Line
	5100 3850 5100 3950
Connection ~ 5100 3850
Wire Wire Line
	5800 3550 5800 3650
Wire Wire Line
	5800 3650 5800 3750
Connection ~ 5800 3650
Wire Wire Line
	5800 3750 5800 3850
Connection ~ 5800 3750
Wire Wire Line
	5800 3850 5800 3950
Connection ~ 5800 3850
Wire Wire Line
	6500 3550 6500 3650
Wire Wire Line
	6500 3650 6500 3750
Connection ~ 6500 3650
Wire Wire Line
	6500 3750 6500 3850
Connection ~ 6500 3750
Wire Wire Line
	6500 3850 6500 3950
Connection ~ 6500 3850
Wire Wire Line
	7200 3550 7200 3650
Wire Wire Line
	7200 3650 7200 3750
Connection ~ 7200 3650
Wire Wire Line
	7200 3750 7200 3850
Connection ~ 7200 3750
Wire Wire Line
	7200 3850 7200 3950
Connection ~ 7200 3850
$Comp
L Device:R R1
U 1 1 61A7FEA9
P 5700 4600
F 0 "R1" V 5493 4600 50  0000 C CNN
F 1 "4K7" V 5584 4600 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder" V 5630 4600 50  0001 C CNN
F 3 "~" H 5700 4600 50  0001 C CNN
	1    5700 4600
	0    1    1    0   
$EndComp
$Comp
L Device:R R0
U 1 1 61A81AF9
P 5700 4250
F 0 "R0" V 5493 4250 50  0000 C CNN
F 1 "4K7" V 5584 4250 50  0000 C CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.20x1.40mm_HandSolder" V 5630 4250 50  0001 C CNN
F 3 "~" H 5700 4250 50  0001 C CNN
	1    5700 4250
	0    1    1    0   
$EndComp
Wire Wire Line
	5550 4250 5300 4250
Wire Wire Line
	5300 3850 5100 3850
Wire Wire Line
	5550 4600 5300 4600
Wire Wire Line
	5300 3850 5300 4250
Connection ~ 5300 4250
Wire Wire Line
	5300 4250 5300 4600
Wire Wire Line
	5850 4250 6650 4250
Wire Wire Line
	6650 4250 6650 3850
Wire Wire Line
	6650 3850 6500 3850
Wire Wire Line
	5850 4600 7350 4600
Wire Wire Line
	7350 4600 7350 3850
Wire Wire Line
	7350 3850 7200 3850
$EndSCHEMATC
