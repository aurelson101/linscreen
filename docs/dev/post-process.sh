# Only run this script from the Makefile

shopt -s globstar
cd output

# Classes backlink to the ClassList in their breadcrumbs. We use the ClassIndex
# instead.
rm -rf linscreen/annotated
ln -sf classes linscreen/annotated

# Hide 'Edit this page button' from the auto-generated docs pages
# It would be better to change the button to link to the file on github, but
# it seems like too much work right now.
sed -i 's|title="Edit this page"|& style="display: none !important"|' linscreen/*/*.html

# MkDoxy adds Qt classes into the class hierarchy. We don't want that.
sed -i 's|<li><strong>class</strong> <strong>Q[^<]*</strong>  </li>||' linscreen/*/*.html

# vim: filetype=bash
