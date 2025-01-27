
#include <math.h>
#include "m_pd.h"

#define HALF_PI (3.14159265358979323846 * 0.5)

static t_class *xfade_class;

typedef struct _xfade{
    t_object    x_obj;
    t_inlet    *x_inlet_mix;
    int         n_in;
    int         channels;
    int         n_out;
    t_sample   *buffer;
    t_sample  **in;
    t_sample  **out;
}t_xfade;

static t_int *xfade_perform(t_int *w){
    t_xfade *x = (t_xfade *)(w[1]);
    int n = (int)(w[2]);
    t_sample* mix = (t_sample *) (w[3]);
    t_sample mix_f;
    int i, j;
    t_sample* out;
    for(j = 0; j < n; j++){
        mix_f = *mix++; // Mix value [-1: left (A) input, 1: right (B) input]
        if(mix_f > 1)
            mix_f = 1;
        if(mix_f < -1)
            mix_f = -1;
        mix_f = (mix_f + 1) * 0.5; // Mix from 0 to 1
        for(i = 0; i < x->n_in; i++)
            x->buffer[i] = (t_sample) x->in[i][j];
        for(i = 0; i < x->channels; i++){
            out = (t_sample *)(x->out[i]);
            out[j] = (x->buffer[i] * (mix_f == 1 ? 0 : cos(mix_f * HALF_PI)))
                + (x->buffer[i+x->channels] * sin(mix_f * HALF_PI));
        }
    }
    return(w+4);
}

static void xfade_dsp(t_xfade *x, t_signal **sp){
    int n;
    t_sample **dummy = x->in;
    for(n = 0; n < x->n_in; n++)
        *dummy++ = sp[n]->s_vec;
    dummy = x->out;
    for(n = 0; n < x->n_out; n++)
        *dummy++ = sp[n + x->n_in + 1]->s_vec;
    dsp_add(xfade_perform, 3, x, sp[0]->s_n, sp[x->n_in]->s_vec);
}

static void xfade_free(t_xfade *x){
     inlet_free(x->x_inlet_mix);
     freebytes(x->in, x->n_in * sizeof(t_sample *));
     freebytes(x->out, x->n_out * sizeof(t_sample *));
     freebytes(x->buffer,x->n_in * sizeof( * x->buffer));
}

static void *xfade_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_xfade *x = (t_xfade *)pd_new(xfade_class);
    t_float f1 = 1, f2 = 0;
    if(ac && av->a_type == A_FLOAT){
        f1 = av->a_w.w_float;
        ac--; av++;
        if(ac && av->a_type == A_FLOAT)
            f2 = av->a_w.w_float;
    }
    x->channels = (int) f1;
    if(x->channels < 1)
        x->channels = 1;
    if(x->channels > 64)
        x->channels = 64;
    t_float init_mix = f2;
    init_mix = init_mix < -1 ? -1 : init_mix >= 1 ? 1 : init_mix; // clipping mix input
    x->n_in =  x->channels * 2;
    x->in = (t_sample **)getbytes(x->n_in * sizeof(t_sample *));
    int i = x->n_in;
    while(i--)
        x->in[i] = 0;
    x->n_out = x->channels;
    x->out = (t_sample **)getbytes(x->n_in * sizeof(t_sample *));
    i = x->n_out;
    while(i--)
        x->out[i] = 0;
    x->buffer = getbytes(x->n_in * sizeof( * x->buffer));
    for(i = 0; i < x->n_in - 1; i++)
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    x->x_inlet_mix = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_mix, init_mix);
    for(i = 0; i < x->n_out; i++)
        outlet_new(&x->x_obj, &s_signal);
    return(void *)x;
}

void xfade_tilde_setup(void){
    xfade_class = class_new(gensym("xfade~"), (t_newmethod)xfade_new,
        (t_method)xfade_free, sizeof(t_xfade), 0, A_GIMME, 0);
    class_addmethod(xfade_class, nullfn, gensym("signal"), 0);
    class_addmethod(xfade_class, (t_method)xfade_dsp, gensym("dsp"), A_CANT, 0);
}
