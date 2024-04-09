#include "schifra_reed_solomon.h"

int main()
{
    /* Finite Field Parameters */
    const std::size_t field_descriptor                =   8;
    const std::size_t generator_polynomial_index      = 120;
    const std::size_t generator_polynomial_root_count =  32;

    /* Reed Solomon Code Parameters */
    const std::size_t code_length = 255;
    const std::size_t fec_length  =  32;
    const std::size_t data_length = code_length - fec_length;

    /* Instantiate Finite Field and Generator Polynomials */
    const schifra::galois::field field(field_descriptor,
                                                  schifra::galois::primitive_polynomial_size06,
                                                  schifra::galois::primitive_polynomial06);

    schifra::galois::field_polynomial generator_polynomial(field);

    if (!schifra::make_sequential_root_generator_polynomial(
              field,
              generator_polynomial_index,
              generator_polynomial_root_count,
              generator_polynomial))
    {
        std::cout << "Error - Failed to create sequential root generator!" << std::endl;
        return 1;
    }

    /* Instantiate Encoder (Channel Encoder) */
    typedef schifra::reed_solomon::encoder<code_length,fec_length> encoder_t;
    const encoder_t encoder(field,generator_polynomial);

    /* Instantiate Decoder (Channel Decoder) */
    typedef schifra::reed_solomon::decoder<code_length,fec_length> decoder_t;
    const decoder_t decoder(field,generator_polynomial_index);

    /* Instantiate RS Block For Codec */
    schifra::reed_solomon::block<code_length,fec_length> block;

    /* Fill the block with data */
    for(std::size_t i = 0; i < data_length; ++i) block[i] = data[i];

    /* Encode block */
    if (!encoder.encode(block))
    {
        std::cout << "Error - Critical decoding failure! "
                     << "Msg: " << block.error_as_string() << std::endl;
        return 1;
    }

    /* Add errors into the encoded block */
    block[  0] ^= 1;
    block[ 10] ^= 1;
    block[ 20] ^= 1;
    block[ 30] ^= 1;
    block[ 40] ^= 1;
    block[ 50] ^= 1;
    block[ 60] ^= 1;
    block[ 70] ^= 1;
    block[ 80] ^= 1;
    block[ 90] ^= 1;
    block[100] ^= 1;

    /* Decode block */
    if (!decoder.decode(block))
    {
        std::cout << "Error - Critical decoding failure! "
                     << "Msg: " << block.error_as_string() << std::endl;
        return 1;
    }

    /* Block should now be corrected and original data restored */
    return 0;
}