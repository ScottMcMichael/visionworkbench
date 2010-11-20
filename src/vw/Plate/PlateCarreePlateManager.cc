// __BEGIN_LICENSE__
// Copyright (C) 2006-2010 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__

#include <vw/Plate/PlateCarreePlateManager.h>
#include <vw/Cartography/GeoReference.h>
#include <vw/Cartography/GeoTransform.h>
#include <vw/Image/Filter.h>

using namespace vw::platefile;
using namespace vw;

template <class PixelT>
void PlateCarreePlateManager<PixelT>::transform_image(
                          cartography::GeoReference const& georef,
                          ImageViewRef<PixelT>& image,
                          TransformRef& txref, int& level ) const {
  // First .. correct input transform if need be
  cartography::GeoReference input_georef = georef;
  if ( boost::contains(input_georef.proj4_str(),"+proj=longlat") ) {
    Matrix3x3 transform = input_georef.transform();
    // Correct if it is so far right it is not visible.
    if ( transform(0,2) > 180 )
      transform(0,2) -= 360;
    if ( input_georef.pixel_to_lonlat(Vector2(image.cols()-1,0))[0] < -180.0 )
      transform(0,2) += 360;
    input_georef.set_transform(transform);
  }

  // Create temporary transform to work out the resolution
  cartography::GeoReference output_georef;
  output_georef.set_datum(input_georef.datum());
  int resolution = 256;
  cartography::GeoTransform geotx( input_georef, output_georef );
  // Calculate the best resolution at 5 different points in the image
  Vector2 res_pixel[5];
  res_pixel[0] = Vector2( image.cols()/2, image.rows()/2 );
  res_pixel[1] = Vector2( image.cols()/2 + image.cols()/4,
                          image.rows()/2 );
  res_pixel[2] = Vector2( image.cols()/2 - image.cols()/4,
                          image.rows()/2 );
  res_pixel[3] = Vector2( image.cols()/2,
                          image.rows()/2 + image.rows()/4 );
  res_pixel[4] = Vector2( image.cols()/2,
                          image.rows()/2 - image.rows()/4 );
  int res;
  for(int i=0; i < 5; i++) {
    res = cartography::output::kml::compute_resolution(geotx, res_pixel[i]);
    if( res > resolution ) resolution = res;
  }

  // Round the resolution to the nearest power of two.  The
  // base of the pyramid is 2^8 or 256x256 pixels.
  level = static_cast<int>(ceil(log(resolution) / log(2))) - 8;
  output_georef = this->georeference( level );

  // Rebuild the transform
  geotx = cartography::GeoTransform( input_georef, output_georef );
  BBox2i output_bbox = geotx.forward_bbox(bounding_box(image));
  vw_out() << "\t    Placing image at level " << level
           << " with bbox " << output_bbox << "\n"
           << "\t    (Total KML resolution at this level =  "
           << resolution << " pixels.)\n";

  // Perform transform and rewrite to input
  ImageViewRef<PixelT> holding =
    transform( image, geotx, ZeroEdgeExtension(),
               BicubicInterpolation() );
  txref = TransformRef(geotx);
  image = holding;
}

template <class PixelT>
cartography::GeoReference
PlateCarreePlateManager<PixelT>::georeference( int level ) const {
  int tile_size = this->m_platefile->default_tile_size();
  int resolution = (1<<level)*tile_size;

  cartography::GeoReference r;
  r.set_pixel_interpretation(cartography::GeoReference::PixelAsArea);

  // Set projecion space to be between -180 and 180.
  Matrix3x3 transform;
  transform(0,0) = 360.0 / resolution;
  transform(0,2) = -180.0;
  transform(1,1) = -360.0 / resolution;
  transform(1,2) = 180.0;
  transform(2,2) = 1;
  r.set_transform(transform);

  return r;
}

template <class PixelT>
void PlateCarreePlateManager<PixelT>::generate_mipmap_tile(
                          int col, int row, int level,
                          Transaction transaction_id, bool preblur) const {
  // Create an image large enough to store all of the child nodes
  int tile_size = this->m_platefile->default_tile_size();
  ImageView<PixelT> super(2*tile_size, 2*tile_size);

  // Iterate over the children, gathering them and (recursively)
  // regenerating them if necessary.
  for( int j=0; j<2; ++j ) {
    for( int i=0; i<2; ++i ) {
      try {
        int child_col = 2*col+i;
        int child_row = 2*row+j;
        vw_out(VerboseDebugMessage, "platefile") << "Reading tile "
                                                 << child_col << " "
                                                 << child_row
                                                 << " @  " << (level+1) << "\n";
        ImageView<PixelT> child;
        this->m_platefile->read(child, child_col, child_row, level+1,
                                transaction_id, true); // exact_transaction
        crop(super,tile_size*i,tile_size*j,tile_size,tile_size) = child;
      } catch (TileNotFoundErr &e) { /*Do Nothing*/}
    }
  }

  // We subsample after blurring with a standard 2x2 box filter.
  std::vector<float> kernel(2);
  kernel[0] = kernel[1] = 0.5;

  ImageView<PixelT> new_tile;
  if (preblur)
    new_tile = subsample( separable_convolution_filter( super, kernel,
                                                        kernel, 1, 1,
                                                        ConstantEdgeExtension() ), 2);
  else
    new_tile = subsample( super, 2 );

  if (!is_transparent(new_tile)) {
    vw_out(VerboseDebugMessage, "platefile") << "Writing " << col << " " << row
                                             << " @ " << level << "\n";
    this->m_platefile->write_update(new_tile, col, row, level, transaction_id);
  }
}

// Explicit template instantiation
namespace vw {
namespace platefile {

#define VW_INSTANTIATE_PLATE_CARREE_TYPES(PIXELT)                            \
  template void                                                              \
  PlateCarreePlateManager<PIXELT >::transform_image(                         \
                                    cartography::GeoReference const& georef, \
                                    ImageViewRef<PIXELT >& image,            \
                                    TransformRef& txref, int& level ) const; \
  template cartography::GeoReference                                         \
  PlateCarreePlateManager<PIXELT >::georeference( int level ) const;         \
  template void                                                              \
  PlateCarreePlateManager<PIXELT >::generate_mipmap_tile(                    \
                                    int col, int row, int level,             \
                                    Transaction transaction_id, bool preblur) const;

  VW_INSTANTIATE_PLATE_CARREE_TYPES(PixelGrayA<uint8>)
  VW_INSTANTIATE_PLATE_CARREE_TYPES(PixelGrayA<int16>)
  VW_INSTANTIATE_PLATE_CARREE_TYPES(PixelGrayA<float32>)
  VW_INSTANTIATE_PLATE_CARREE_TYPES(PixelRGBA<uint8>)

}}
