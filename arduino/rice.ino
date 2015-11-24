#define MAX_RICE_PARAM 20

typedef struct rice_encode_context
{
  uint32_t buffer;
  uint32_t filled;
  uint32_t bits;
} rice_encode_context;

typedef struct rice_decode_context
{
  uint32_t buffer;
  uint32_t filled;
  uint32_t bytes;
} rice_decode_context;

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  if (Serial.available() > 0)
  {
    //Variables
    int32_t i, j = 0;
    int16_t opt_param;
    size_t read;
    uint32_t bits = 1, num_elements = 1, passed = 1;
    
    //Buffers
    int32_t o_buffer[10];//Buffer to hold original data
    uint32_t p_buffer[10];
    uint32_t encoded[10];//Buffer to hold compressed data
    uint32_t d_buffer[10];
    int32_t buffer2[10];//Buffer to hold decompressed data
    
    //Generate random numbers between 0 - 400 and fill input buffer
    for (i = 0; i < 10; i++)
      o_buffer[i] = rand() % 400;

    //Transform everything to positive
    signed_to_unsigned(10, o_buffer, p_buffer);

    //Calculate optimum rice parameter
    opt_param = get_opt_rice_param(p_buffer, 10, &bits);

    //Encode
    bits = rice_encode_block(opt_param, p_buffer, 10, encoded);

    //Decode
    int32_t out_elements = rice_decode_block(opt_param, encoded, 10, d_buffer);

    //Transform back to original
    unsigned_to_signed(10, d_buffer, buffer2);
    
    for(i = 0; i < out_elements; i++)
    {
       if(o_buffer[i] != buffer2[i])
      {
         passed = 0;
         break;
      } 
    }
    
    if(passed)
      Serial.println("Passed. Encoding & Decoding OK :)");
    else
      Serial.println("Failed. There is a bug :(");
    
    //For debug purpose
    
    //for(i = 0; i < 10; i++)
      //fprintf(stderr,"Input[%d] = %d, Output[%d] = %d\n",i,o_buffer[i],i,buffer2[i]);
      
    Serial.println("Give me a byte to run again");
    Serial.println();
    
    //Wait for input
    Serial.read();
  }
}

/*
*Golomb-Rice algorithm is designed to work with unsigned numbers
*This function maps all positive integers to even postive numbers
*and negative intgers to odd positive numbers
->data_size = number of elements in input & output buffer
->input = input buffer which contains +ve/-ve values
<->output = output buffer which contains only positive values
*/
int32_t signed_to_unsigned(const uint32_t data_size, const int32_t *input, uint32_t *output)
{
  for (uint32_t i = 0; i < data_size; i++)
    (input[i] < 0) ? (output[i] = (-(input[i] << 1)) - 1) : (output[i] = input[i] << 1);
  return (0);
}

/*
*This function does exactly the reverse of the above function
*Maps all odd numbers to negative integers
*and even numbers to positive integers
->data_size = number of elements in input & output buffer
->input = input buffer which contains only positive values
<->output = output buffer which contains +ve/-ve values
*/
int32_t unsigned_to_signed(const uint32_t data_size, const uint32_t *input, int32_t *output)
{
  for (uint32_t i = 0; i < data_size; i++)
    ((input[i] & 0x01) == 0x01) ? (output[i] = -((input[i] + 1) >> 1)) : (output[i] = input[i] >> 1);
  return (0);
}

/*calculate optimum rice parameter for encoding a block of data
->data = buffer containing positive input data
->data_size = number of elements in input buffer
<->req_bits = bits required to compress data using the optimum rice parameter
returns the optimum rice parameter which is between 0 - MAX_RICE_PARAM
*/
uint16_t get_opt_rice_param(const uint32_t *data, int32_t data_size, uint32_t *req_bits)
{
  int32_t i = 0, j;
  uint32_t bits[MAX_RICE_PARAM];
  uint32_t temp;
  uint16_t best_index;

  for (i = 0; i < MAX_RICE_PARAM; i++)
  {
    temp = 0;
    for (j = 0; j < data_size; j++)
    {
      //data[j]/2^k;
      temp += data[j] >> i;
      temp += 1;
      temp += i;
    }
    bits[i] = temp;
  }

  //Get the minimum parameter
  temp = bits[0];
  best_index = 0;
  *req_bits = bits[0];

  for (i = 1; i < MAX_RICE_PARAM; i++)
  {
    if (bits[i] < temp)
    {
      temp = bits[i];
      *req_bits = bits[i];
      best_index = (int16_t)i;
    }
  }
  return best_index;
}

/*
*Encode a block of data using Golomb-Rice coding
->param = rice parameter to be used for compressing the input
->input = input buffer containing only positive values
->size = size of input buffer
<->encoded = buffer to store compressed values
returns the number of bits required to store compressed data.
return value should be equal to the *req_bits of 'get_opt_rice_param()'
*/
uint32_t rice_encode_block(int16_t param, const uint32_t *input, int32_t size, uint32_t *encoded)
{
  int32_t i, j, temp, written = 0;
  uint32_t bits;

  rice_encode_context context;
  rice_encode_context *ctx = &context;

  ctx->buffer = 0;
  ctx->filled = 0;
  ctx->bits = 0;

  for (i = 0; i < size; i++)
  {
    temp = input[i] >> param;

    //Write out 'temp' number of ones
    for (j = 0; j < temp; j++)
    {
      ctx->buffer = ctx->buffer | (0x0001 << ctx->filled);
      ctx->bits++;
      if (ctx->filled == 31)
      {
        encoded[written++] = ctx->buffer;
        ctx->buffer = 0;
        ctx->filled = 0;
      }
      else
        ctx->filled++;
    }

    //Write out a zero bit
    ctx->buffer = ctx->buffer | (0x0000 << ctx->filled);
    ctx->bits++;
    if (ctx->filled == 31)
    {
      encoded[written++] = ctx->buffer;
      ctx->buffer = 0;
      ctx->filled = 0;
    }
    else
      ctx->filled++;

    //Write out the last 'param' bits of input[i]
    for (j = param - 1; j >= 0; j--)
    {
      ctx->buffer = ctx->buffer | (((input[i] >> j) & 0x0001) << ctx->filled);
      ctx->bits++;
      if (ctx->filled == 31)
      {
        encoded[written++] = ctx->buffer;
        ctx->buffer = 0;
        ctx->filled = 0;
      }
      else
        ctx->filled++;
    }
  }

  //Flush buffer if not empty
  if (ctx->filled != 0)
    encoded[written] = ctx->buffer;

  bits = ctx->bits;
  return (bits);
}

/*Decode a block of data encoded using Golomb-Rice coding
->param = rice parameter
<->encoded = pointer to buffer containing encoded values
->count = Number of decoded elements
<->decoded = Buffer to store decompressed values
returns Number of decoded elements
*/
uint32_t rice_decode_block(int16_t param, const uint32_t *encoded, int32_t count, uint32_t *decoded)
{
  int32_t i = 0, j = 0, q, k = 0;
  int16_t tmp;
  rice_decode_context context;
  rice_decode_context *ctx = &context;
  ctx->filled = 0;

  while (k < count)
  {
    //Count ones until a zero is encountered
    q = 0;
    while (1)
    {
      if (ctx->filled == 0)
      {
        ctx->filled = 32;
        ctx->buffer = encoded[i++];
      }
      tmp = ctx->buffer & 0x0001;
      ctx->buffer = ctx->buffer >> 1;
      ctx->filled--;
      if (tmp == 0)
        break;
      q++;
    }
    decoded[k] = q << param; //x = q * 2^k

    //Read out the last 'param' bits of input
    for (j = param - 1; j >= 0; j--)
    {
      if (ctx->filled == 0)
      {
        ctx->filled = 32;
        ctx->buffer = encoded[i++];
      }
      tmp = ctx->buffer & 0x0001;
      ctx->buffer = ctx->buffer >> 1;
      ctx->filled--;
      decoded[k] = decoded[k] | (tmp << j);
    }

    k++;
  }

  return (k);
}
