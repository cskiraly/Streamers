struct input_desc;

struct input_desc *input_open(const char *fname);
void input_close(struct input_desc *s);
int input_get(struct input_desc *s, struct chunk *c);
