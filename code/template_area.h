
#if !defined Template_Area_Name
#error Template_Area_Name needs to be defined befor including area_template.h
#endif

#if !defined Template_Area_Vector_Type
#error Template_Area_Vector_Type needs to be defined befor including area_template.h
#endif

#if !defined Template_Area_Struct_Is_Declared

struct Template_Area_Name {
    AREA_TEMPLATE_VECTOR_TYPE min, size;
};

#undef Template_Area_Struct_Is_Declared

#endif

INTERNAL Template_Area_Name make_by_border(Template_Area_Vector_Type min, Template_Area_Vector_Type max)
{
    Template_Area_Name result;
    result.min  = min;
    result.size = max - min;
    
    return result;
}

INTERNAL Template_Area_Name merge(Template_Area_Name a, Template_Area_Name b)
{
    Template_Area_Name result;
    
    for (u32 i = 0; i < ARRAY_COUNT(result.min.values); ++i)
        result.min[i] = min(a.min[i], b.min[i]);
    
    for (u32 i = 0; i < ARRAY_COUNT(result.size.values); ++i)
        result.size[i] = max(a.min[i] + a.size[i], b.min[i] + b.size[i]) - result.min[i];
    
    return result;
}

INTERNAL bool contains(Template_Area_Name area, Template_Area_Vector_Type point)
{
    Template_Area_Vector_Type max = area.min + area.size;
    
    for (u32 i = 0; i < ARRAY_COUNT(point.values); ++i)
    {
        if (area.min[i] > point[i] || max[i] < point[i])
            return false;
    }
    
    return true;
}

INTERNAL bool intersects(Template_Area_Name a, Template_Area_Name b)
{
    for (u32 i = 0; i < ARRAY_COUNT(a.min.values); ++i)
    {
        if (a.min[i] <= b.min[i])
        {
            if (b.min[i] >= a.min[i] + a.size[i])
                return false;
        }
        else
        {
            if (a.min[i] >= b.min[i] + b.size[i])
                return false;
        }
    }
    
    return true;
}

INTERNAL Template_Area_Name intersection(OUTPUT bool *is_valid, Template_Area_Name a, Template_Area_Name b)
{
    Template_Area_Name result;
    
    for (u32 i = 0; i < ARRAY_COUNT(a.min.values); ++i)
    {
        result.min[i] = max(a.min[i], b.min[i]);
        auto max = min(a.min[i] + a.size[i], b.min[i] + b.size[i]);
        result.size[i] = max - result.min[i];
        
        if (result.size[i] <= 0) {
            *is_valid = false;
            return {};
        }
    }
    
    *is_valid = true;
    return result;
}

#undef Template_Area_Name
#undef Template_Area_Vector_Type