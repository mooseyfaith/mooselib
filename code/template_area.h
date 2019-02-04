
#if !defined Template_Area_Name
#error Template_Area_Name needs to be defined befor including area_template.h
#endif

#if !defined Template_Area_Vector_Type
#error Template_Area_Vector_Type needs to be defined befor including area_template.h
#endif

#if !defined Template_Area_Data_Type
#error Template_Area_Data_Type needs to be defined befor including area_template.h
#endif

#if !defined Template_Area_Struct_Is_Declared

struct Template_Area_Name {
    AREA_TEMPLATE_VECTOR_TYPE min, size;
    bool is_valid;
};

#undef Template_Area_Struct_Is_Declared

#endif

INTERNAL Template_Area_Name CHAIN(make_empty_, Template_Area_Name) () {
    Template_Area_Name result;
    
    for (u32 i = 0; i < ARRAY_COUNT(result.size.values); i++) {
        result.min.values[i] = CHAIN(Template_Area_Data_Type, _max);
        result.size.values[i] = 0;
    }
    
    return result;
}

INTERNAL auto volume(Template_Area_Name area) {
    auto result = area.size[0];
    
    for (u32 i = 1; i < ARRAY_COUNT(area.size.values); i++)
        result*= area.size[1];
    
    return result;
}

INTERNAL Template_Area_Name make_by_border(Template_Area_Vector_Type min, Template_Area_Vector_Type max)
{
    Template_Area_Name result;
    result.min  = min;
    result.size = max - min;
    result.is_valid = volume(result) > 0.0f;
    
    return result;
}

INTERNAL Template_Area_Name merge(Template_Area_Name a, Template_Area_Name b)
{
    if (a.is_valid) {
        if (b.is_valid) {
            Template_Area_Name result;
            
            for (u32 i = 0; i < ARRAY_COUNT(result.min.values); ++i)
                result.min[i] = min(a.min[i], b.min[i]);
            
            for (u32 i = 0; i < ARRAY_COUNT(result.size.values); ++i)
                result.size[i] = max(a.min[i] + a.size[i], b.min[i] + b.size[i]) - result.min[i];
            
            result.is_valid = true;
            return result;
        }
        else {
            return a;
        }
    }
    else {
        return b;
    }
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

INTERNAL Template_Area_Name intersection(Template_Area_Name a, Template_Area_Name b)
{
    Template_Area_Name result;
    
    assert(a.is_valid && b.is_valid);
    
    for (u32 i = 0; i < ARRAY_COUNT(a.min.values); ++i)
    {
        result.min[i] = max(a.min[i], b.min[i]);
        auto max = min(a.min[i] + a.size[i], b.min[i] + b.size[i]);
        result.size[i] = max - result.min[i];
        
        if (result.size[i] <= 0)
            return {};
    }
    
    result.is_valid = true;
    return result;
}

#undef Template_Area_Name
#undef Template_Area_Vector_Type