#ifndef II42_AM_OPTIONS_H
#define II42_AM_OPTIONS_H

#include "postgres.h"
#include "utils/rel.h"

#include "ii42_core.h"
#include "ii42_semantic_bmp.h"

typedef enum ii42_am_consistency
{
    II42_AM_CONSISTENCY_REALTIME = 0,
    II42_AM_CONSISTENCY_EVENTUAL,
    II42_AM_CONSISTENCY_MANUAL
} ii42_am_consistency;

typedef enum ii42_am_runtime_precision
{
    II42_AM_RUNTIME_PRECISION_FP16 = 0,
    II42_AM_RUNTIME_PRECISION_FP32
} ii42_am_runtime_precision;

typedef enum ii42_am_build_mode
{
    II42_AM_BUILD_MODE_IDS = 0,
    II42_AM_BUILD_MODE_TEXT_ARRAY_SINGLE,
    II42_AM_BUILD_MODE_TEXT_ARRAY_MULTI,
    II42_AM_BUILD_MODE_SCALAR_SINGLE,
    II42_AM_BUILD_MODE_SCALAR_MULTI
} ii42_am_build_mode;

typedef struct ii42_am_text_policy
{
    bool lowercase;
    bool stem_english;
    bool fold_diacritics;
    const char *raw_stopwords;
} ii42_am_text_policy;

typedef struct ii42_am_policy_recommendation
{
    const char *profile;
    const char *confidence;
    const char *recommended_options;
    const char *recommended_consistency;
    bool matches_current;
    bool refresh_now;
    uint32 docs;
    uint64 pending_total;
    const char *reason;
} ii42_am_policy_recommendation;

void ii42_init_reloptions(void);
bytea *ii42_amoptions(Datum reloptions, bool validate);
int ii42_am_index_natts(Relation index_relation);

int ii42_am_index_total_natts(Relation index_relation);

int ii42_am_index_scope_natts(Relation index_relation);
bool ii42_am_is_multicol_index(Relation index_relation);
Oid ii42_am_source_type(Relation index_relation);
void ii42_am_validate_source_type(Oid source_type);
bool ii42_am_source_type_is_text_array(Oid source_type);
bool ii42_am_source_type_is_scalar_text(Oid source_type);
bool ii42_am_source_type_is_textlike(Oid source_type);
Oid ii42_am_require_textlike_source_type(
    Relation index_relation,
    const char *context
);
ii42_am_build_mode ii42_am_build_mode_from_source_type(
    Oid source_type,
    int natts
);
const char *ii42_am_consistency_name(int consistency);
const char *ii42_am_runtime_precision_name(int precision);
const char *ii42_am_semantic_impact_precision_name(int precision);
void ii42_am_get_policy_recommendation(
    Relation index_relation,
    const char *profile,
    ii42_am_policy_recommendation *recommendation
);
int ii42_am_get_consistency(Relation index_relation);
void ii42_am_validate_relation_policy(Relation index_relation);
int ii42_am_auto_preload_priority(Relation index_relation);
bool ii42_am_field_aware_enabled(Relation index_relation);
bool ii42_am_sae_enabled(Relation index_relation);
int ii42_am_get_runtime_precision(Relation index_relation);
ii42_semantic_impact_precision ii42_am_get_semantic_impact_precision(
    Relation index_relation
);
double ii42_am_semantic_alpha_mass(Relation index_relation);
const char *ii42_am_relation_model_path(Relation index_relation);
void ii42_am_read_text_policy(
    Relation index_relation,
    ii42_am_text_policy *policy_out
);
void ii42_am_read_params(
    Relation index_relation,
    ii42_params *params_out,
    bool *create_empty_token_out
);

#endif
