THERMOCAM=" \
  thermocam \
"

for THERMOCAM in $THERMOCAM; do
  make -C $THERMOCAM
done


