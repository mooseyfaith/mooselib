
#if !defined AREA_TEMPLATE_NAME
#error AREA_TEMPLATE_NAME needs to be defined befor including area_template.h
#endif

#if !defined AREA_TEMPLATE_VECTOR_TYPE
#error AREA_TEMPLATE_VECTOR_TYPE needs to be defined befor including area_template.h
#endif

struct AREA_TEMPLATE_NAME {
    AREA_TEMPLATE_VECTOR_TYPE min, size;
};

struct CHAIN(AREA_TEMPLATE_NAME, _Min_Max) {
    AREA_TEMPLATE_VECTOR_TYPE min, max;
};

CHAIN(AREA_TEMPLATE_NAME, _Min_Max) to_area_min_max(AREA_TEMPLATE_NAME area) {
    return { area.min, area.min + area.size };
}

AREA_TEMPLATE_NAME to_area(CHAIN(AREA_TEMPLATE_NAME, _Min_Max) area_min_max) {
    return { area_min_max.min, area_min_max.max + area_min_max.min };
}

AREA_TEMPLATE_NAME merge(AREA_TEMPLATE_NAME a, AREA_TEMPLATE_NAME b) {
    AREA_TEMPLATE_NAME result;
    
    for (u32 i = 0; i < ARRAY_COUNT(result.min.values); ++i) {
        result.min[i] = MIN(a.min[i], b.min[i]);
    }
    
    for (u32 i = 0; i < ARRAY_COUNT(result.size.values); ++i) {
        result.size[i] = MAX(a.min[i] + a.size[i], b.min[i] + b.size[i]) - result.min[i];
    }
    
    return result;
}

bool contains(AREA_TEMPLATE_NAME area, AREA_TEMPLATE_VECTOR_TYPE point) {
    AREA_TEMPLATE_VECTOR_TYPE max = area.min + area.size;
    
    for (u32 i = 0; i < ARRAY_COUNT(point.values); ++i) {
        if (area.min[i] > point[i] || max[i] < point[i])
            return false;
    }
    
    return true;
}

bool intersects(AREA_TEMPLATE_NAME a, AREA_TEMPLATE_NAME b) {
    for (u32 i = 0; i < ARRAY_COUNT(a.min.values); ++i) {
        if (a.min[i] <= b.min[i]) {
            if (b.min[i] >= a.min[i] + a.size[i])
                return false;
        }
        else {
            if (a.min[i] >= b.min[i] + b.size[i])
                return false;
        }
    }
    
    return true;
}

AREA_TEMPLATE_NAME intersection(OUTPUT bool *is_valid, AREA_TEMPLATE_NAME a, AREA_TEMPLATE_NAME b) {
    AREA_TEMPLATE_NAME result;
    
    for (u32 i = 0; i < ARRAY_COUNT(a.min.values); ++i)
    {
        result.min[i] = MAX(a.min[i], b.min[i]);
        auto max = MIN(a.min[i] + a.size[i], b.min[i] + b.size[i]);
        result.size[i] = max - result.min[i];
        
        if (result.size[i] <= 0) {
            *is_valid = false;
            return {};
        }
    }
    
    *is_valid = true;
    return result;
}

#undef AREA_TEMPLATE_NAME
#undef AREA_TEMPLATE_VECTOR_TYPE