/*
 * blueMail offline mail reader
 * uncollect routine source code (to be included)

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


/* bool */
uncollect (const char *fname, long offset, const char *dir, bool verbose)
{
  FILE *src, *dest;
  int files, i, j, c;
  char name[MYMAXPATH];
  long len, rlen;
  bool success = false;

  clearDirectory(dir);

  if ((src = fopen(fname, "rb")))
  {
    fseek(src, offset, SEEK_SET);

    if (fscanf(src, COLL_MAGIC "%d:", &files) == 1)
    {
      char *buffer = new char[BLOCKLEN];

      for (i = 0; i < files; i++)
      {
        j = 0;

        do name[j] = (c = fgetc(src));
        while (++j < MYMAXPATH && c != '\0');
        name[--j] = '\0';

        if (fscanf(src, "%ld:", &len) != 1) break;

        if ((dest = fopen(name, "wb")))
        {
          if (verbose) fprintf(stdout, "extracting %s\n", name);

          while (len > 0)
          {
            rlen = (len < BLOCKLEN ? len : BLOCKLEN);
            fread(buffer, sizeof(char), rlen, src);
            fwrite(buffer, sizeof(char), rlen, dest);
            len -= rlen;
          }
          fclose(dest);
        }
        else break;
      }

      success = (i == files);
      delete[] buffer;
    }

    fclose(src);
  }

  return success;
}
