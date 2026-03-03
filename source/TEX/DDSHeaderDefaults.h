#pragma once

void SetDDSHeaderDefaults_DXT5(DDS_HEADER *ddsheader);
void SetDDSHeaderDefaults_DXT1(DDS_HEADER *ddsheader);
void SetDDSHeaderDefaults_RGBA(DDS_HEADER *ddsheader);
void SetDDSHeaderDefaults_RGB(DDS_HEADER *ddsheader);
void SetDDSHeaderDefaults_3Dc(DDS_HEADER *ddsheader);
void SetDDSHeaderDefaults_BC7(DDS_HEADER *ddsheader, DDS_HEADER_DXT10 *dx10Header);