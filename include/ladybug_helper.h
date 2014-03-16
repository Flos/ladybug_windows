#pragma once
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include "configuration_helper.h"
#include "timing.h"
#include <locale.h>
#include "error.h"

LadybugError initCamera( LadybugContext context);
LadybugError configureLadybugForPanoramic(LadybugContext context);
LadybugError startLadybug(LadybugContext context);
LadybugError saveImages(LadybugContext context, int i);
void ladybugImagetoDisk(LadybugImage *image, std::string filename);
void extractCalibration(LadybugContext context, LadybugImage *image);