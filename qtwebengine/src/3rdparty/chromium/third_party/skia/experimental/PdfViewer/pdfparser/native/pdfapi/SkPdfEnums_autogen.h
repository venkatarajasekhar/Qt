/*
 * Copyright 2013 Google Inc.

 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkPdfEnums_DEFINED
#define SkPdfEnums_DEFINED

enum SkPdfNativeObjectType {
  kNone_SkPdfNativeObjectType = 0,
  kDictionary_SkPdfNativeObjectType,
  kALinkAnnotationDictionary_SkPdfNativeObjectType,
  kActionDictionary_SkPdfNativeObjectType,
  kAlternateImageDictionary_SkPdfNativeObjectType,
  kAnnotationActionsDictionary_SkPdfNativeObjectType,
  kAnnotationDictionary_SkPdfNativeObjectType,
  kAppearanceCharacteristicsDictionary_SkPdfNativeObjectType,
  kAppearanceDictionary_SkPdfNativeObjectType,
  kApplicationDataDictionary_SkPdfNativeObjectType,
  kArtifactsDictionary_SkPdfNativeObjectType,
  kAttributeObjectDictionary_SkPdfNativeObjectType,
  kBeadDictionary_SkPdfNativeObjectType,
  kBlockLevelStructureElementsDictionary_SkPdfNativeObjectType,
  kBorderStyleDictionary_SkPdfNativeObjectType,
  kBoxColorInformationDictionary_SkPdfNativeObjectType,
  kBoxStyleDictionary_SkPdfNativeObjectType,
  kCIDFontDescriptorDictionary_SkPdfNativeObjectType,
  kCIDFontDictionary_SkPdfNativeObjectType,
  kCIDSystemInfoDictionary_SkPdfNativeObjectType,
  kCMapDictionary_SkPdfNativeObjectType,
  kCalgrayColorSpaceDictionary_SkPdfNativeObjectType,
  kCalrgbColorSpaceDictionary_SkPdfNativeObjectType,
  kCatalogDictionary_SkPdfNativeObjectType,
  kCcittfaxdecodeFilterDictionary_SkPdfNativeObjectType,
  kCheckboxFieldDictionary_SkPdfNativeObjectType,
  kChoiceFieldDictionary_SkPdfNativeObjectType,
  kComponentsWithMetadataDictionary_SkPdfNativeObjectType,
  kDctdecodeFilterDictionary_SkPdfNativeObjectType,
  kDeviceNColorSpaceDictionary_SkPdfNativeObjectType,
  kDocumentCatalogActionsDictionary_SkPdfNativeObjectType,
  kDocumentInformationDictionary_SkPdfNativeObjectType,
  kEmbeddedFileParameterDictionary_SkPdfNativeObjectType,
  kEmbeddedFileStreamDictionary_SkPdfNativeObjectType,
  kEmbeddedFontStreamDictionary_SkPdfNativeObjectType,
  kEncodingDictionary_SkPdfNativeObjectType,
  kEncryptedEmbeddedFileStreamDictionary_SkPdfNativeObjectType,
  kEncryptionCommonDictionary_SkPdfNativeObjectType,
  kFDFCatalogDictionary_SkPdfNativeObjectType,
  kFDFDictionary_SkPdfNativeObjectType,
  kFDFFieldDictionary_SkPdfNativeObjectType,
  kFDFFileAnnotationDictionary_SkPdfNativeObjectType,
  kFDFNamedPageReferenceDictionary_SkPdfNativeObjectType,
  kFDFPageDictionary_SkPdfNativeObjectType,
  kFDFTemplateDictionary_SkPdfNativeObjectType,
  kFDFTrailerDictionary_SkPdfNativeObjectType,
  kFieldDictionary_SkPdfNativeObjectType,
  kFileAttachmentAnnotationDictionary_SkPdfNativeObjectType,
  kFileSpecificationDictionary_SkPdfNativeObjectType,
  kFileTrailerDictionary_SkPdfNativeObjectType,
  kFontDescriptorDictionary_SkPdfNativeObjectType,
  kFontDictionary_SkPdfNativeObjectType,
  kType0FontDictionary_SkPdfNativeObjectType,
  kType1FontDictionary_SkPdfNativeObjectType,
  kMultiMasterFontDictionary_SkPdfNativeObjectType,
  kTrueTypeFontDictionary_SkPdfNativeObjectType,
  kType3FontDictionary_SkPdfNativeObjectType,
  kType1FontDictionary__End_SkPdfNativeObjectType,
  kFontDictionary__End_SkPdfNativeObjectType,
  kFormFieldActionsDictionary_SkPdfNativeObjectType,
  kFreeTextAnnotationDictionary_SkPdfNativeObjectType,
  kFunctionCommonDictionary_SkPdfNativeObjectType,
  kGoToActionDictionary_SkPdfNativeObjectType,
  kGraphicsStateDictionary_SkPdfNativeObjectType,
  kGroupAttributesDictionary_SkPdfNativeObjectType,
  kHideActionDictionary_SkPdfNativeObjectType,
  kIccProfileStreamDictionary_SkPdfNativeObjectType,
  kIconFitDictionary_SkPdfNativeObjectType,
  kImportDataActionDictionary_SkPdfNativeObjectType,
  kInkAnnotationDictionary_SkPdfNativeObjectType,
  kInlineLevelStructureElementsDictionary_SkPdfNativeObjectType,
  kInteractiveFormDictionary_SkPdfNativeObjectType,
  kJavascriptActionDictionary_SkPdfNativeObjectType,
  kJavascriptDictionary_SkPdfNativeObjectType,
  kJbig2DecodeFilterDictionary_SkPdfNativeObjectType,
  kLabColorSpaceDictionary_SkPdfNativeObjectType,
  kLaunchActionDictionary_SkPdfNativeObjectType,
  kLineAnnotationDictionary_SkPdfNativeObjectType,
  kListAttributeDictionary_SkPdfNativeObjectType,
  kLzwdecodeAndFlatedecodeFiltersDictionary_SkPdfNativeObjectType,
  kMacOsFileInformationDictionary_SkPdfNativeObjectType,
  kMarkInformationDictionary_SkPdfNativeObjectType,
  kMarkedContentReferenceDictionary_SkPdfNativeObjectType,
  kMarkupAnnotationsDictionary_SkPdfNativeObjectType,
  kMetadataStreamDictionary_SkPdfNativeObjectType,
  kMovieActionDictionary_SkPdfNativeObjectType,
  kMovieActivationDictionary_SkPdfNativeObjectType,
  kMovieAnnotationDictionary_SkPdfNativeObjectType,
  kMovieDictionary_SkPdfNativeObjectType,
  kNameDictionary_SkPdfNativeObjectType,
  kNameTreeNodeDictionary_SkPdfNativeObjectType,
  kNamedActionsDictionary_SkPdfNativeObjectType,
  kNumberTreeNodeDictionary_SkPdfNativeObjectType,
  kObjectReferenceDictionary_SkPdfNativeObjectType,
  kOpiVersionDictionary_SkPdfNativeObjectType,
  kOutlineDictionary_SkPdfNativeObjectType,
  kOutlineItemDictionary_SkPdfNativeObjectType,
  kPDF_XOutputIntentDictionary_SkPdfNativeObjectType,
  kPSXobjectDictionary_SkPdfNativeObjectType,
  kPageLabelDictionary_SkPdfNativeObjectType,
  kPageObjectActionsDictionary_SkPdfNativeObjectType,
  kPageObjectDictionary_SkPdfNativeObjectType,
  kPagePieceDictionary_SkPdfNativeObjectType,
  kPageTreeNodeDictionary_SkPdfNativeObjectType,
  kPopUpAnnotationDictionary_SkPdfNativeObjectType,
  kPrinterMarkAnnotationDictionary_SkPdfNativeObjectType,
  kPrinterMarkFormDictionary_SkPdfNativeObjectType,
  kRadioButtonFieldDictionary_SkPdfNativeObjectType,
  kReferenceDictionary_SkPdfNativeObjectType,
  kRemoteGoToActionDictionary_SkPdfNativeObjectType,
  kResetFormActionDictionary_SkPdfNativeObjectType,
  kResourceDictionary_SkPdfNativeObjectType,
  kRubberStampAnnotationDictionary_SkPdfNativeObjectType,
  kSeparationDictionary_SkPdfNativeObjectType,
  kShadingDictionary_SkPdfNativeObjectType,
  kType1ShadingDictionary_SkPdfNativeObjectType,
  kType2ShadingDictionary_SkPdfNativeObjectType,
  kType3ShadingDictionary_SkPdfNativeObjectType,
  kType4ShadingDictionary_SkPdfNativeObjectType,
  kType5ShadingDictionary_SkPdfNativeObjectType,
  kType6ShadingDictionary_SkPdfNativeObjectType,
  kShadingDictionary__End_SkPdfNativeObjectType,
  kSignatureDictionary_SkPdfNativeObjectType,
  kSoftMaskDictionary_SkPdfNativeObjectType,
  kSoundActionDictionary_SkPdfNativeObjectType,
  kSoundAnnotationDictionary_SkPdfNativeObjectType,
  kSoundObjectDictionary_SkPdfNativeObjectType,
  kSourceInformationDictionary_SkPdfNativeObjectType,
  kSquareOrCircleAnnotation_SkPdfNativeObjectType,
  kStandardSecurityHandlerDictionary_SkPdfNativeObjectType,
  kStandardStructureDictionary_SkPdfNativeObjectType,
  kStreamCommonDictionary_SkPdfNativeObjectType,
  kStructureElementAccessDictionary_SkPdfNativeObjectType,
  kStructureElementDictionary_SkPdfNativeObjectType,
  kStructureTreeRootDictionary_SkPdfNativeObjectType,
  kSubmitFormActionDictionary_SkPdfNativeObjectType,
  kTableAttributesDictionary_SkPdfNativeObjectType,
  kTextAnnotationDictionary_SkPdfNativeObjectType,
  kTextFieldDictionary_SkPdfNativeObjectType,
  kThreadActionDictionary_SkPdfNativeObjectType,
  kThreadDictionary_SkPdfNativeObjectType,
  kTransitionDictionary_SkPdfNativeObjectType,
  kTransparencyGroupDictionary_SkPdfNativeObjectType,
  kTrapNetworkAnnotationDictionary_SkPdfNativeObjectType,
  kTrapNetworkAppearanceStreamDictionary_SkPdfNativeObjectType,
  kType0FunctionDictionary_SkPdfNativeObjectType,
  kType10HalftoneDictionary_SkPdfNativeObjectType,
  kType16HalftoneDictionary_SkPdfNativeObjectType,
  kType1HalftoneDictionary_SkPdfNativeObjectType,
  kType1PatternDictionary_SkPdfNativeObjectType,
  kType2FunctionDictionary_SkPdfNativeObjectType,
  kType2PatternDictionary_SkPdfNativeObjectType,
  kType3FunctionDictionary_SkPdfNativeObjectType,
  kType5HalftoneDictionary_SkPdfNativeObjectType,
  kType6HalftoneDictionary_SkPdfNativeObjectType,
  kURIActionDictionary_SkPdfNativeObjectType,
  kURIDictionary_SkPdfNativeObjectType,
  kURLAliasDictionary_SkPdfNativeObjectType,
  kVariableTextFieldDictionary_SkPdfNativeObjectType,
  kViewerPreferencesDictionary_SkPdfNativeObjectType,
  kWebCaptureCommandDictionary_SkPdfNativeObjectType,
  kWebCaptureCommandSettingsDictionary_SkPdfNativeObjectType,
  kWebCaptureDictionary_SkPdfNativeObjectType,
  kWebCaptureImageSetDictionary_SkPdfNativeObjectType,
  kWebCaptureInformationDictionary_SkPdfNativeObjectType,
  kWebCapturePageSetDictionary_SkPdfNativeObjectType,
  kWidgetAnnotationDictionary_SkPdfNativeObjectType,
  kWindowsLaunchActionDictionary_SkPdfNativeObjectType,
  kXObjectDictionary_SkPdfNativeObjectType,
  kImageDictionary_SkPdfNativeObjectType,
  kSoftMaskImageDictionary_SkPdfNativeObjectType,
  kImageDictionary__End_SkPdfNativeObjectType,
  kType1FormDictionary_SkPdfNativeObjectType,
  kXObjectDictionary__End_SkPdfNativeObjectType,
  kDictionary__End_SkPdfNativeObjectType,
};

class SkPdfDictionary;
class SkPdfXObjectDictionary;
class SkPdfFontDictionary;
class SkPdfTrueTypeFontDictionary;
class SkPdfStreamCommonDictionary;
class SkPdfLzwdecodeAndFlatedecodeFiltersDictionary;
class SkPdfCcittfaxdecodeFilterDictionary;
class SkPdfJbig2DecodeFilterDictionary;
class SkPdfDctdecodeFilterDictionary;
class SkPdfFileTrailerDictionary;
class SkPdfEncryptionCommonDictionary;
class SkPdfStandardSecurityHandlerDictionary;
class SkPdfCatalogDictionary;
class SkPdfPageTreeNodeDictionary;
class SkPdfPageObjectDictionary;
class SkPdfNameDictionary;
class SkPdfResourceDictionary;
class SkPdfNameTreeNodeDictionary;
class SkPdfNumberTreeNodeDictionary;
class SkPdfFunctionCommonDictionary;
class SkPdfType0FunctionDictionary;
class SkPdfType2FunctionDictionary;
class SkPdfType3FunctionDictionary;
class SkPdfFileSpecificationDictionary;
class SkPdfEmbeddedFileStreamDictionary;
class SkPdfEmbeddedFileParameterDictionary;
class SkPdfMacOsFileInformationDictionary;
class SkPdfGraphicsStateDictionary;
class SkPdfCalgrayColorSpaceDictionary;
class SkPdfCalrgbColorSpaceDictionary;
class SkPdfLabColorSpaceDictionary;
class SkPdfIccProfileStreamDictionary;
class SkPdfDeviceNColorSpaceDictionary;
class SkPdfType1PatternDictionary;
class SkPdfType2PatternDictionary;
class SkPdfShadingDictionary;
class SkPdfType1ShadingDictionary;
class SkPdfType2ShadingDictionary;
class SkPdfType3ShadingDictionary;
class SkPdfType4ShadingDictionary;
class SkPdfType5ShadingDictionary;
class SkPdfType6ShadingDictionary;
class SkPdfImageDictionary;
class SkPdfAlternateImageDictionary;
class SkPdfType1FormDictionary;
class SkPdfGroupAttributesDictionary;
class SkPdfReferenceDictionary;
class SkPdfPSXobjectDictionary;
class SkPdfType1FontDictionary;
class SkPdfType3FontDictionary;
class SkPdfEncodingDictionary;
class SkPdfCIDSystemInfoDictionary;
class SkPdfCIDFontDictionary;
class SkPdfCMapDictionary;
class SkPdfType0FontDictionary;
class SkPdfFontDescriptorDictionary;
class SkPdfCIDFontDescriptorDictionary;
class SkPdfEmbeddedFontStreamDictionary;
class SkPdfType1HalftoneDictionary;
class SkPdfType6HalftoneDictionary;
class SkPdfType10HalftoneDictionary;
class SkPdfType16HalftoneDictionary;
class SkPdfType5HalftoneDictionary;
class SkPdfSoftMaskDictionary;
class SkPdfSoftMaskImageDictionary;
class SkPdfTransparencyGroupDictionary;
class SkPdfViewerPreferencesDictionary;
class SkPdfOutlineDictionary;
class SkPdfOutlineItemDictionary;
class SkPdfPageLabelDictionary;
class SkPdfThreadDictionary;
class SkPdfBeadDictionary;
class SkPdfTransitionDictionary;
class SkPdfAnnotationDictionary;
class SkPdfBorderStyleDictionary;
class SkPdfAppearanceDictionary;
class SkPdfTextAnnotationDictionary;
class SkPdfALinkAnnotationDictionary;
class SkPdfFreeTextAnnotationDictionary;
class SkPdfLineAnnotationDictionary;
class SkPdfSquareOrCircleAnnotation;
class SkPdfMarkupAnnotationsDictionary;
class SkPdfRubberStampAnnotationDictionary;
class SkPdfInkAnnotationDictionary;
class SkPdfPopUpAnnotationDictionary;
class SkPdfFileAttachmentAnnotationDictionary;
class SkPdfSoundAnnotationDictionary;
class SkPdfMovieAnnotationDictionary;
class SkPdfWidgetAnnotationDictionary;
class SkPdfActionDictionary;
class SkPdfAnnotationActionsDictionary;
class SkPdfPageObjectActionsDictionary;
class SkPdfFormFieldActionsDictionary;
class SkPdfDocumentCatalogActionsDictionary;
class SkPdfGoToActionDictionary;
class SkPdfRemoteGoToActionDictionary;
class SkPdfLaunchActionDictionary;
class SkPdfWindowsLaunchActionDictionary;
class SkPdfThreadActionDictionary;
class SkPdfURIActionDictionary;
class SkPdfURIDictionary;
class SkPdfSoundActionDictionary;
class SkPdfMovieActionDictionary;
class SkPdfHideActionDictionary;
class SkPdfNamedActionsDictionary;
class SkPdfInteractiveFormDictionary;
class SkPdfFieldDictionary;
class SkPdfVariableTextFieldDictionary;
class SkPdfAppearanceCharacteristicsDictionary;
class SkPdfCheckboxFieldDictionary;
class SkPdfRadioButtonFieldDictionary;
class SkPdfTextFieldDictionary;
class SkPdfChoiceFieldDictionary;
class SkPdfSignatureDictionary;
class SkPdfSubmitFormActionDictionary;
class SkPdfResetFormActionDictionary;
class SkPdfImportDataActionDictionary;
class SkPdfJavascriptActionDictionary;
class SkPdfFDFTrailerDictionary;
class SkPdfFDFCatalogDictionary;
class SkPdfFDFDictionary;
class SkPdfEncryptedEmbeddedFileStreamDictionary;
class SkPdfJavascriptDictionary;
class SkPdfFDFFieldDictionary;
class SkPdfIconFitDictionary;
class SkPdfFDFPageDictionary;
class SkPdfFDFTemplateDictionary;
class SkPdfFDFNamedPageReferenceDictionary;
class SkPdfFDFFileAnnotationDictionary;
class SkPdfSoundObjectDictionary;
class SkPdfMovieDictionary;
class SkPdfMovieActivationDictionary;
class SkPdfDocumentInformationDictionary;
class SkPdfMetadataStreamDictionary;
class SkPdfComponentsWithMetadataDictionary;
class SkPdfPagePieceDictionary;
class SkPdfApplicationDataDictionary;
class SkPdfStructureTreeRootDictionary;
class SkPdfStructureElementDictionary;
class SkPdfMarkedContentReferenceDictionary;
class SkPdfObjectReferenceDictionary;
class SkPdfStructureElementAccessDictionary;
class SkPdfAttributeObjectDictionary;
class SkPdfMarkInformationDictionary;
class SkPdfArtifactsDictionary;
class SkPdfStandardStructureDictionary;
class SkPdfBlockLevelStructureElementsDictionary;
class SkPdfInlineLevelStructureElementsDictionary;
class SkPdfListAttributeDictionary;
class SkPdfTableAttributesDictionary;
class SkPdfWebCaptureInformationDictionary;
class SkPdfWebCaptureDictionary;
class SkPdfWebCapturePageSetDictionary;
class SkPdfWebCaptureImageSetDictionary;
class SkPdfSourceInformationDictionary;
class SkPdfURLAliasDictionary;
class SkPdfWebCaptureCommandDictionary;
class SkPdfWebCaptureCommandSettingsDictionary;
class SkPdfBoxColorInformationDictionary;
class SkPdfBoxStyleDictionary;
class SkPdfPrinterMarkAnnotationDictionary;
class SkPdfPrinterMarkFormDictionary;
class SkPdfSeparationDictionary;
class SkPdfPDF_XOutputIntentDictionary;
class SkPdfTrapNetworkAnnotationDictionary;
class SkPdfTrapNetworkAppearanceStreamDictionary;
class SkPdfOpiVersionDictionary;
class SkPdfMultiMasterFontDictionary;

#endif  // SkPdfEnums_DEFINED
