# This script takes a file that was created using morph_data.* scripts,
# and recreates the original file.
BEGIN { for (i = 0; i <= 26; i++) { k = i + 65; ct[sprintf("%c", k)] = i; }}
{
  n = split($0, s, /\+/);
  printf "%s\t", s[1];
  if ((d = length(s[1]) - ct[substr(s[2], 1, 1)]) > 0)
    printf "%s", substr(s[1], 1, d);
  if (length(s[2]) > 1) printf "%s", substr(s[2], 2);
  printf "\t%s\n", s[3];
}

  
