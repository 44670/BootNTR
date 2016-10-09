#include "main.h"

extern char		*g_primary_error;

bool    fileExists(const char *path)
{
    int exists;

    if (!path) goto error;
    exists = access(path, F_OK);
    if (exists == 0)
        return (true);
error:
    return (false);
}

int		copy_file(char *old_filename, char  *new_filename)
{
	FILE		*old_file = NULL;
	FILE		*new_file = NULL;
	t_BLOCK		value;

	old_file = fopen(old_filename, "rb");
	new_file = fopen(new_filename, "wb");
	value = (t_BLOCK) { {0, 0, 0, 0} };
	check_prim(!old_file, FILE_COPY_ERROR);
	if (!new_file)
	{
		fclose(old_file);
		check_prim(!new_file, FILE_COPY_ERROR);
	}
	while (1)
	{
		fread(&value, 4, 4, old_file);
		if (!feof(old_file))
			fwrite(&value, 4, 4, new_file);
		else
			break;
	}
	fclose(new_file);
	fclose(old_file);
	return  (0);
error:
    remove("sdmc:/ntr.bin");
	return (RESULT_ERROR);
}
