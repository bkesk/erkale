/*
 *                This source code is part of
 * 
 *                     E  R  K  A  L  E
 *                             -
 *                       DFT from Hel
 *
 * Written by Jussi Lehtola, 2010-2011
 * Copyright (c) 2010-2011, Jussi Lehtola
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


#ifndef ERKALE_ELEMENTS
#define ERKALE_ELEMENTS

#include <string>

/// Names of elements
const std::string element_names[]={
  "",
  "Hydrogen","Helium",
  "Lithium","Beryllium","Boron","Carbon","Nitrogen","Oxygen","Fluorine","Neon",
  "Sodium","Magnesium","Aluminium","Silicon","Phosphorus","Sulfur","Chlorine","Argon",
  "Potassium","Calcium","Scandium","Titanium","Vanadium","Chromium","Manganese","Iron","Cobalt","Nickel","Copper","Zinc","Gallium","Germanium","Arsenic","Selenium","Bromine","Krypton",
  "Rubidium","Strontium","Yttrium","Zirconium","Niobium","Molybdenum","Technetium","Ruthenium","Rhodium","Palladium","Silver","Cadmium","Indium","Tin","Antimony","Tellurium","Iodine","Xenon"
};

/// Symbols of elements
const std::string element_symbols[]={
  "",
  "H","He",
  "Li","Be","B","C","N","O","F","Ne",
  "Na","Mg","Al","Si","P","S","Cl","Ar",
  "K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr",
  "Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe"
};

/// The row of the elements
const int element_row[]={
  0,
  0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

/// Number of core electrons of the elements
const int Nel_core[]={
  0,
  0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
};

/// Number of supported elements
const int element_N=55;

/// Atomic masses from http://www.chem.qmul.ac.uk/iupac/AtWt/index.html
/// Technetium has no stable isotopes, set mass to -1e-10
const double atomic_masses[]={
  0.0,
  1.008000, 4.002602,
  6.940000, 9.012182, 10.810000, 12.011000, 14.007000, 15.999000, 18.998403, 20.179700,
  22.989769, 24.305000, 26.981539, 28.085000, 30.973762, 32.060000, 35.450000, 39.948000,
  39.098300, 40.078000, 44.955912, 47.867000, 50.941500, 51.996100, 54.938045, 55.845000, 58.933195, 58.693400, 63.546000, 65.380000, 69.723000, 72.630000, 74.921600, 78.960000, 79.904000, 83.798000,
  85.467800, 87.620000, 88.905850, 91.224000, 92.906380, 95.960000, -1e-10, 101.070000, 102.905500, 106.420000, 107.868200, 112.411000, 114.818000, 118.710000, 121.760000, 127.600000, 126.904470, 131.293000
};


/// Get nuclear charge of element
int get_Z(std::string el);

#endif
