# Joint Pin {#elements_joint_pin}

[TOC]

One cylinder along its axis: every screw, dowel or pin is a pin, alone or a child of its connector.

## Constructors

```cpp
JointPinElement(const Line& axis, double radius, double chord_tolerance = 0.05);

const Line& axis() const;
```

## Along an axis

A 10 mm radius pin along a 200 mm vertical axis.

![](elements/element_joint_pin.png)

\include{lineno} elements/element_joint_pin.cpp

## Typical Dimensions

| Pin | Diameter mm | Length mm | Source |
| --- | --- | --- | --- |
| Steel dowel | 8 | 60 to 140 | [Rothoblaas STA](https://www.rothoblaas.com/attachments/1410/STA-en.pdf) |
| Steel dowel | 12 | 60 to 400 | [Rothoblaas STA](https://www.rothoblaas.com/attachments/1410/STA-en.pdf) |
| Steel dowel | 16 | 80 to 500 | [Rothoblaas STA](https://www.rothoblaas.com/attachments/1410/STA-en.pdf) |
| Steel dowel | 20 | 120 to 500 | [Rothoblaas STA](https://www.rothoblaas.com/attachments/1410/STA-en.pdf) |
| Hardwood dowel in a dowel-laminated panel | 20 | 620 | [Sohm ETA](https://werbewind-login.com/files/?f=ETA+DiagonalD%C3%BCbelholz_DE.pdf&cnt=document&cntid=C90735F7-9409-BC88-4A84-1433DF69A68C) |
| Hardwood peg of a carpentry joint, as tested | 24, 32, 40 | | [IRB](https://www.irbnet.de/daten/rswb/94009500091.pdf) |
| Beech nail | 3.7, 4.7 to 5.3 | 38 to 90 | [LignoLoc](https://cdn.shopify.com/s/files/1/0728/4565/6395/files/LignoLoc-Flyer.pdf) |
| Screw, threaded over its length | 7, 9, 11 | 80 to 400, 160 to 600, 150 to 1000 | [Rothoblaas VGZ](https://www.rothoblaas.com/products/fastening/screws/vgz) |
| Screw, threaded over its length | 9, 11, 13 | 100 to 600, 80 to 1000, 80 to 1500 | [Rothoblaas VGS](https://rothoblaas.com/products/fastening/screws/vgs) |
| Screw, threaded over its length | 9, 13 | to 1000 | [SFS WR-T](https://www.dibt.de/pdf_storage/2022/ETA-12!0062%288.06.03-113!22%29e.pdf) |
| Threaded rod | 16, 20 | to 3000 | [SFS WB-T](https://ca.sfs.com/content/files/2510%20-%20CA/5%20Documents/Timber%20brochure%20extracted%20sections/WB%20fastener%20catalog%20pages.pdf) |
