#!/usr/bin/perl
while(<>)
{
	s/D3D11_SRV_DIMENSION_/D3D10_1_SRV_DIMENSION_/g;
	s/D3D11/D3D10/g;
	s/D3D10_SIGNATURE_PARAMETER_DESC/D3D11_SIGNATURE_PARAMETER_DESC/g;
	s/D3D_FEATURE_LEVEL_/D3D10_FEATURE_LEVEL_/g;
	s/D3D_FEATURE_LEVEL/D3D10_FEATURE_LEVEL1/g;
	s/^#define API 11/#define API 10/;
	s/^(#include "d3d1)1(_[^.]*)(.h")/${1}0$2.generated$3/;
	print $_;
}
