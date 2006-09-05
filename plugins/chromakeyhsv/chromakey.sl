uniform sampler2D tex;
uniform float red;
uniform float green;
uniform float blue;
uniform float in_slope;
uniform float out_slope;
uniform float tolerance;
uniform float tolerance_in;
uniform float tolerance_out;
uniform float sat;
uniform float min_s;
uniform float min_s_in;
uniform float min_s_out;
uniform float min_v;
uniform float min_v_in;
uniform float min_v_out;
uniform float max_v;
uniform float max_v_in;
uniform float max_v_out;
uniform float spill_threshold;
uniform float spill_amount;
uniform float alpha_offset;
uniform float hue_key;
uniform float saturation_key;
uniform float value_key;

void main()
{
	vec4 color = texture2D(tex, gl_TexCoord[0].st);
/* Contribution to alpha from each component */
	float alpha_value = 1.0;
	float alpha_value_max = 1.0;
	float alpha_hue = 1.0;
	float alpha_saturation = 1.0;
	bool has_match = true;
	vec4 color2;
	
/* Convert to HSV */
	color2 = yuv_to_rgb(color);
	color2 = rgb_to_hsv(color2);

/* Hue is completely out of range */
	if (tolerance == 0.0)
	    alpha_hue = 1.0;
	else
	if (abs(color2.r - hue_key) < tolerance_in * 180.0)
		alpha_hue = 0.0;
	else 
	if ((out_slope != 0.0 ) && (abs(color2.r - hue_key) < tolerance * 180.0)) 
/* If using slope, scale alpha between 0 and 1 / 2 */
		alpha_hue = abs(color2.r - hue_key) / tolerance / 360.0; 
	else 
	if (abs(color2.r - hue_key) < tolerance_out * 180.0) 
/* If no slope, scale alpha between 1/2 and 1 */
		alpha_hue = abs(color2.r - hue_key) / tolerance_out / 360.0; 
	else
		has_match = false;	

/* Test saturation */
	if (has_match) 
	{
		if (min_s == 0.0)
			alpha_saturation = 0.0;
		else 
		if ( color2.g - sat >= min_s_in )
			alpha_saturation = 0.0;
		else 
		if ((out_slope != 0.0) && ( color2.g - sat > min_s ) )
			alpha_saturation = (color2.g - sat - min_s) / (min_s * 2.0);
		else 
		if ( color2.g - sat > min_s_out ) 
			alpha_saturation = (color2.g - sat - min_s_out) / (min_s_out * 2.0);
		else 
			has_match = false;
	}

/* Test value over minimum */
	if (has_match)
	{
	    if (min_v == 0.0)
			alpha_value = 0.0;
		else 
		if ( color2.b >= min_v_in )
			alpha_value = 0.0;
		else 
		if ((out_slope != 0.0) && ( color2.b > min_v ) )
			alpha_value = (color2.b - min_v) / (min_v * 2.0);
		else 
		if ( color2.b > min_v_out )
			alpha_value = (color2.b - min_v_out) / (min_v_out * 2.0);
		else
			has_match = false;
	}

/* Test value under maximum */
	if (has_match)
	{
	    if (max_v == 0.0)
			alpha_value_max = 1.0;
	    else 
		if (color2.b <= max_v_in)
			alpha_value_max = 0.0;
	    else 
		if ((out_slope != 0.0) && (color2.b < max_v))
			alpha_value_max = (color2.b - max_v) / (max_v * 2.0);
	    else 
		if (color2.b < max_v_out)
			alpha_value_max = (color2.b - max_v_out) / (max_v_out * 2.0);
	    else
			has_match = false;
	}

/* Take largest component as the alpha */
	if (has_match)
	   	color2.a = max (max (alpha_hue, alpha_value), max (alpha_saturation, alpha_value_max));

/* Spill light processing */
	if ((abs(color2.r - hue_key) < spill_threshold * 180.0) || 
	    ((abs(color2.r - hue_key) > 360.0) && 
	    (abs(color2.r - hue_key) - 360.0 < spill_threshold * 180.0)))
	{
/* Modify saturation based on hue contribution */
	    color2.g = color2.g * 
			spill_amount * 
			abs(color2.r - hue_key) / 
			(spill_threshold * 180.0);

/* convert back to native colormodel */
		color2 = hsv_to_rgb(color2);
		color.rgb = rgb_to_yuv(color2).rgb;
	}


/* Convert mask into image */
	gl_FragColor = show_mask(color, color2);
}


