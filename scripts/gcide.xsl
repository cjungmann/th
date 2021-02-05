<?xml version="1.0" encoding="UTF-8" ?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output
      method="text"
      encoding="utf-8"/>

  <xsl:variable name="nl"><xsl:text>
</xsl:text></xsl:variable>

  <xsl:template match="dictionary">
    <xsl:text>dictionary</xsl:text>
    <xsl:value-of select="$nl" />
    <xsl:apply-templates select="*" />
    <xsl:text>DONE!!!!</xsl:text>
    <xsl:value-of select="$nl" />
  </xsl:template>

  <xsl:template match="*"></xsl:template>

  <xsl:template match="ent|pos|sn|def|source">
    <xsl:value-of select="concat(local-name(), ': ', ., $nl)" />
  </xsl:template>


</xsl:stylesheet>

