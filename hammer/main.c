#include <hammer/hammer.h>
#include <hammer/internal.h>
#include <hammer/glue.h>
#include <stdio.h>

struct FileType {
  char*   major_brand;
  char*   major_brand_version;
  char*   compatible_brands;
};

HParsedToken *ftyp_action(const HParseResult *p, void *user_data) {
  printf("token type: %d\n", p->ast->token_type);
  if( p->ast->token_type == TT_SEQUENCE) {
    char* buffer = h_arena_malloc(p->ast->seq->arena, p->ast->seq->used);
    for (size_t i=0; i<p->ast->seq->used; ++i) {
      buffer[i] = H_CAST_UINT(p->ast->seq->elements[i]);
    }

    HParser* brand    = h_repeat_n(h_choice(h_ch_range('0', '9'), h_ch_range('a', 'z'), NULL), 4);
    HParser* brands   = h_many(brand);
    HParser* ftyp     = h_sequence(brand, h_uint32(), brands, NULL);
    HParseResult *res = h_parse(ftyp, buffer + 8, p->ast->seq->used - 8);

    if(res) {
      printf("parsed ftyp\n");

      struct FileType* ft         = h_arena_malloc(p->ast->seq->arena, sizeof(struct FileType));
      ft->major_brand             = h_arena_malloc(p->ast->seq->arena, 5);
      ft->major_brand_version     = h_arena_malloc(p->ast->seq->arena, 5);
      ft->compatible_brands       = h_arena_malloc(p->ast->seq->arena, (p->ast->seq->used - 8) + 1);

      for (size_t i=0; i < 4; ++i) {
        ft->major_brand[i] = H_CAST_UINT(p->ast->seq->elements[i]);
      }
      ft->major_brand[4] = 0;
      for (size_t i=0; i < 4; ++i) {
        ft->major_brand_version[i] = H_CAST_UINT(p->ast->seq->elements[i+4]);
      }
      ft->major_brand_version[4] = 0;
      for (size_t i=0; i < (p->ast->seq->used - 8); ++i) {
        ft->compatible_brands[i] = H_CAST_UINT(p->ast->seq->elements[i+8]);
      }
      printf("major brand: %s\n major version: %04X\ncompatible brands: %s\n",
        ft->major_brand, ft->major_brand_version, ft->compatible_brands);

      h_arena_free(p->ast->seq->arena, ft->major_brand);
      h_arena_free(p->ast->seq->arena, ft->major_brand_version);
      h_arena_free(p->ast->seq->arena, ft->compatible_brands);
      h_arena_free(p->ast->seq->arena, ft);
      h_arena_free(p->ast->seq->arena, buffer);
      return res;
    } else {
      printf("did not parse ftyp\n");
      h_arena_free(p->ast->seq->arena, buffer);
    }
  }
  return NULL;
}

HParser* build_parser() {
  HParser *ftyp_tag = h_token("ftyp", 4);
  HParser *moov_tag = h_token("moov", 4);
  HParser *free_tag = h_token("free", 4);
  HParser *skip_tag = h_token("skip", 4);
  HParser *mdra_tag = h_token("mdra", 4);
  HParser *dref_tag = h_token("dref", 4);
  HParser *cmov_tag = h_token("cmov", 4);
  HParser *rmra_tag = h_token("rmra", 4);
  HParser *iods_tag = h_token("iods", 4);
  HParser *mvhd_tag = h_token("mvhd", 4);
  HParser *clip_tag = h_token("clip", 4);
  HParser *trak_tag = h_token("trak", 4);
  HParser *udta_tag = h_token("udta", 4);


  HParser *tag      = h_choice(ftyp_tag, moov_tag, mdra_tag, dref_tag, cmov_tag, rmra_tag, iods_tag, mvhd_tag, clip_tag, trak_tag, udta_tag, NULL);

  //HParser *ftyp_box  = h_length_value(h_left(h_uint32(), ftyp_tag), h_uint8());
  HParser *ftyp_box  = h_action(h_length_value(h_left(h_uint32(), ftyp_tag), h_uint8()), ftyp_action, NULL);
  HParser *free_box  = h_length_value(h_left(h_uint32(), free_tag), h_uint8());
  HParser *mp4_box  = h_choice(ftyp_box, free_tag, NULL);
  //HParser *mp4_box  = h_length_value(h_left(h_uint32(), ftyp), h_uint8());
  //return mp4_box;

  HParser *complete_parser = h_many1(mp4_box);
  return complete_parser;
}

int main(int argc, char *argv[]) {
  uint8_t input[1024];
  size_t inputsize;

  FILE *fp = fopen ( "../small.mp4" , "rb" );
  fseek( fp , 0L , SEEK_END);
  long lSize = ftell( fp );
  rewind( fp );

  const uint8_t* buffer = calloc( 1, lSize+1 );
  if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

  if( 1!=fread((void*)buffer , lSize, 1 , fp) )
    fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);

  printf("got a buffer of %ld bytes\n", lSize);

  HParser *parser = build_parser();
  //HParser *manyparser = h_many1(parser);
  printf("built the parser\n");

  //HParser *parser = h_uint32();
  HParseResult *result = h_parse(parser, buffer, lSize);
  if(result) {
      printf("yay!\n");
  } else {
      printf("nay!\n");
  }

  fclose(fp);
  free(buffer);
}
