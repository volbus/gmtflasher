
/* Used to extract numerical values from xml nodes. It returns the value or -1
 * in case of error and prints the error message
 */
static int
get_xml_node_val (xmlNode *node)
{
  int val, k;
  char c;

  xmlChar *xmlcontent = xmlNodeGetContent (node);
  if (!xmlcontent) {
    printf ("%s:%s:%i: xmlNodeGetContent()= NULL\n", __FILE__,
        __func__, __LINE__);
    return -1;
  }

  k = sscanf((char *) xmlcontent, "%i%c", &val, &c);

  if (k == 1) {
  } else if (k == 2) {
    if (c=='K')
      val *= 1024;
    else if (c=='k')
      val *= 1000;
  } else {
    printf ("%s:%s:%d: error in gmtflasher_devices.xml:%d, "
          "cannot get value \"%s\"\n",
          __FILE__, __func__, __LINE__, node->line, xmlcontent);
    val = -1;
  }

  xmlFree (xmlcontent);
  return val;
}

/* We have 2 main types of µCs: STM8L/STM8S; and we identify them by the start
 * address of the flash control registers block.
 * The function re-/sets the PROG_MODE_STM8L flag, and retruns 0. In case of
 * errror prints the error message and returns -1
 */
static int
set_device_type (xmlNode *node, mcu *uc)
{
  xmlChar *xmlcontent = xmlNodeGetContent (node);
  if (!xmlcontent) {
    printf ("%s:%s:%i: xmlNodeGetContent()= NULL\n", __FILE__,
        __func__, __LINE__);
    return -1;
  }

  if (   !xmlStrcasecmp(xmlcontent, (const xmlChar *) "STM8L")
      || !xmlStrcasecmp(xmlcontent, (const xmlChar *) "STM8AL")
      || !xmlStrcasecmp(xmlcontent, (const xmlChar *) "STM8TL") ) {
    prog_mode |= PROG_MODE_STM8L;
  } else if ( !xmlStrcasecmp(xmlcontent, (const xmlChar *) "STM8S")
      || !xmlStrcasecmp(xmlcontent, (const xmlChar *) "STM8AF") ) {
    prog_mode &= ~PROG_MODE_STM8L;
  } else {
    printf ("%s:%s:%d: error in gmtflasher_devices.xml:%d, "
        "unknown device type \"%s\"\n",
        __FILE__, __func__, __LINE__, node->line, xmlcontent);
    xmlFree (xmlcontent);
    return -1;
  }

  xmlFree (xmlcontent);
  return 0;
}

/* The function searches uc->name in the xml file, and fetches the data for it
 * from the file. If the mcu is not found or any other data is not identified it
 * returns -1, otherwise 0, and the 5 mcu constants are filled in: flash_add,
 * flash_size, eeprom_add, eeprom_size, block_size.
 */
void
Get_Xml_Mcu_Data (mcu *uc)
{
  xmlDoc    *xml_dev_list;
  xmlNode   *element;
  int      k, q;

  xml_dev_list = xmlParseFile("/usr/share/gmtflasher/gmtflasher_devices.xml");
  if (!xml_dev_list)
    exit (EXIT_FAILURE);

  element = xmlDocGetRootElement (xml_dev_list);
  if (!element) {
    printf ("Error in gmtflasher_devices.xml, could not get root element\n");
    goto ret_err;
  }

  if (xmlStrcmp(element->name, (const xmlChar *) "Gmt_Flasher_Data")) {
    printf ("Error in gmtflasher_devices.xml, unknown root element\n");
    goto ret_err;
  }

  element = element->children;
  while (element) {
    if (!xmlStrcmp(element->name, (const xmlChar *) "Devices"))
      break;
    element = element->next;
  }
  if (!element) {
    printf ("Error in gmtflasher_devices.xml, missing Devices node\n");
    goto ret_err;
  }

  k = 0;
  element = element->children;
  while (element) {
    if (!xmlStrcasecmp(element->name, (const xmlChar *) uc->name)) {
      strncpy (uc->name, (char *) element->name, 32);
      uc->name[sizeof(uc->name)] = 0x00;
      xmlNode *mcu_node = element->children;
      while (mcu_node) {
        if (!xmlStrcmp(mcu_node->name, (const xmlChar *) "Device_Type")) {
	  q = set_device_type (mcu_node, uc);
          if (q==-1)
            goto ret_err;
          k |= 0x01;
        } else if (!xmlStrcmp(mcu_node->name, (const xmlChar *) "Flash_Size")) {
          q = get_xml_node_val (mcu_node);
          if (q==-1)
            goto ret_err;
          uc->flash_size = q;
          k |= 0x02;
        } else if (!xmlStrcmp(mcu_node->name, (const xmlChar *) "Block_Size")) {
          q = get_xml_node_val (mcu_node);
          if (q==-1)
            goto ret_err;
          uc->block_size = q;
          k |= 0x04;
        } else if (!xmlStrcmp(mcu_node->name, (const xmlChar *) "Eeprom_Size")) {
          q = get_xml_node_val (mcu_node);
          if (q==-1)
            goto ret_err;
          uc->eeprom_size = q;
          k |= 0x08;
        } else if (!xmlStrcmp(mcu_node->name, (const xmlChar *) "Eeprom_Add")) {
          q = get_xml_node_val (mcu_node);
          if (q==-1)
            goto ret_err;
          uc->eeprom_add = q;
          k |= 0x10;
        }
        mcu_node = mcu_node->next;
      }
      break;
    }
    element = element->next;
  }

  //check if we reached the list end without finding the mcu name
  if (!element) {
    printf ("µC \"%s\" not supported\n", uc->name);
    goto ret_err;
  }

  //check if all data was identified
  if (k != 0x1F) {
    printf ("Error in gmtflasher_devices.xml, could not read all "
        "MCU data\n");
    goto ret_err;
  }

  free (xml_dev_list);
  return;

ret_err:
  free (xml_dev_list);
  exit (EXIT_FAILURE);
}

/* Lists all devices from the gmtflasher_devices.xml file
 */
void
List_Devices (void)
{
  xmlDoc  *xml_dev_list;
  xmlNode *element;

  xml_dev_list = xmlParseFile ("/usr/share/gmtflasher/gmtflasher_devices.xml");
  if (!xml_dev_list)
    exit (EXIT_FAILURE);


  element = xmlDocGetRootElement (xml_dev_list);
  if (!element) {
    printf ("Error in gmtflasher_devices.xml, could not get root "
        "element\n");
    free (xml_dev_list);
    exit (EXIT_FAILURE);
  }

  if ( xmlStrcmp(element->name, (const xmlChar *) "Gmt_Flasher_Data") ) {
    printf ("Error in gmtflasher_devices.xml, unknown root element\n");
    free (xml_dev_list);
    exit (EXIT_FAILURE);
  }

  element = element->children;
  while (element) {
    if (!xmlStrcmp(element->name, (const xmlChar *) "Devices"))
      break;
    element = element->next;
  }
  if (!element) {
    printf ("Error in gmtflasher_devices.xml, missing Devices node\n");
    free (xml_dev_list);
    exit (EXIT_FAILURE);
  }

  element = element->children;
  int i = 0;
  while (element) {
    if (element->type == XML_ELEMENT_NODE) {
      printf ("%s, ", (char *) element->name);
      i++;
      if (i==5) {
        i = 0;
        printf("\n");
      }
    }
    element = element->next;
  }

  if (i)
    printf ("\n");

  free (xml_dev_list);
  return;
}
